# State Machines

Complex operations are decomposed into **ordered stages** tracked in a persistent context structure. Engine functions validate the current stage before advancing; chain functions run the full sequence in one call.

This pattern applies toolkit-wide: reflective loading, classic injection, and future domains (APC injection, evasion modules) must follow the same rules.

---

## Global context pattern

Every major domain context contains:

| Field category | Purpose | Examples |
|---|---|---|
| **DI pointers** | How the operation talks to the OS | `mem_api`, `mod_api`, `proc_api` |
| **Target data** | Operation inputs and live handles | `raw_source`, `target_pid`, `remote_base` |
| **State tracker** | Current pipeline position | `stage` enum |
| **Derived state** | Parsed / mapped artifacts | `snd_pe_parser_t pe`, `snd_pe_target_t target` |

### Loaders vs injection contexts

| | Loaders | Injection |
|---|---|---|
| Context | **Per technique** — `snd_ldr_pe_ctx_t` for reflective PE today | **Shared** — `snd_inj_ctx_t` for all classic (and future) techniques |
| Rationale | Different loader techniques may need incompatible fields | Remote open/write/execute primitives are technique-agnostic |

Future loader techniques add new context types (e.g. `snd_ldr_xyz_ctx_t`). Future injection techniques reuse `snd_inj_ctx_t` and add technique-specific engine headers.

---

## Why state machines

### Granular error handling

Failures return `snd_status_t` with framework `code`, OS `os_error`, and (in DEBUG) expected vs actual stage in `context`. Operators know whether failure happened at allocation, relocation, or thread creation without attaching a debugger.

### Pausable execution

Because all progress lives in the context struct, an implant can stop between engine calls, sleep, and resume. Example: allocate locally → long sleep → apply relocations — as long as `stage` is preserved.

### Enforced ordering

Calling `snd_ldr_pe_resolve_imports` before relocations completes yields `SND_STATUS_INVALID_STAGE_SEQUENCE` instead of corrupting the mapped image.

---

## Reflective loader (`snd_ldr_pe_ctx_t`)

**Header:** `include/sindri/loaders/reflective/engine.h`  
**Chain:** `include/sindri/loaders/reflective/chain.h`

### Stage enum (`snd_ldr_pe_stage_t`)

| Stage | Set by | Meaning |
|---|---|---|
| `SND_STAGE_UNINITIALIZED` | Zero-init | No PE parsed |
| `SND_STAGE_PARSED` | `snd_pe_parse` (via prepare or manual) | Headers validated |
| `SND_STAGE_MEM_ALLOCATED` | `snd_ldr_pe_allocate_and_copy_image` | `local_base` allocated, headers copied |
| `SND_STAGE_SECTIONS_MAPPED` | *(same function)* | Section data copied |
| `SND_STAGE_RELOCATED` | `snd_ldr_pe_apply_relocations` | Base relocations applied |
| `SND_STAGE_IMPORTS_RESOLVED` | `snd_ldr_pe_resolve_imports` | IAT populated |
| `SND_STAGE_READY_FOR_EXECUTION` | `snd_ldr_pe_apply_memory_protections` | Final section protections applied |
| `SND_STAGE_EXECUTED` | `snd_ldr_pe_execute_image` | TLS + entry / DllMain ran |

### Chain vs granular API

**One-shot local load:**

```c
snd_ldr_pe_prepare_image(ctx);   // PARSED → READY_FOR_EXECUTION
snd_ldr_pe_execute_image(ctx);   // TLS + entry (local only)
```

`prepare_image` runs: parse → compat check → alloc/copy → reloc → imports → protections.  
It does **not** run TLS callbacks or the entry point.

**Granular** engine functions (`snd_ldr_pe_apply_relocations`, etc.) exist for manual staging; each validates predecessor stages.

### Execution target (`snd_pe_target_t`)

| Field | Role |
|---|---|
| `local_base` | Allocation in current process (baked image) |
| `execution_base` | Base used for RVAs / entry (equals `local_base` for local load) |
| `delta_offset` | `execution_base - preferred_image_base` for relocations |
| `entry_point` | Resolved VA for DllMain or EXE entry |
| `allocated_size` | Size of mapped image |

For **remote PE injection**, classic chain sets `execution_base = remote_base` after remote allocation so relocations and imports target the remote VA. Local `snd_ldr_pe_execute_image` is blocked when `local_base != execution_base`.

### Local execute path

`snd_ldr_pe_execute_image`:

1. Requires `stage >= SND_STAGE_READY_FOR_EXECUTION`
2. Requires `local_base == execution_base`
3. Runs TLS `DLL_PROCESS_ATTACH`
4. DLL: calls DllMain; EXE: jumps to entry (does not return on success)

Optional post-load export calls in PoCs use `snd_ldr_pe_get_proc_address` + `snd_ffi_execute` (outside the stage machine).

### Detach / cleanup

`snd_ldr_pe_detach_image` — only when `stage == SND_STAGE_EXECUTED` and image is local: DllMain `DLL_PROCESS_DETACH`, TLS detach, `snd_ldr_pe_free_mapped_image`.

---

## Classic injection (`snd_inj_ctx_t`)

**Header:** `include/sindri/injection/context.h`  
**Engine:** `include/sindri/injection/classic/engine.h`  
**Chains:** `include/sindri/injection/classic/chain.h`

### Stage enum (`snd_inj_stage_t`)

| Stage | Set by | Meaning |
|---|---|---|
| `SND_INJ_STAGE_UNINITIALIZED` | Zero-init | No target opened |
| `SND_INJ_STAGE_TARGET_ACQUIRED` | `snd_inj_classic_open_target` | `target_process` handle valid |
| `SND_INJ_STAGE_MEMORY_ALLOCATED` | `snd_inj_classic_alloc_remote` | `remote_base` allocated |
| `SND_INJ_STAGE_PAYLOAD_WRITTEN` | `snd_inj_classic_write_payload` | Bytes copied to target |
| `SND_INJ_STAGE_PROTECTIONS_SET` | `snd_inj_classic_set_protections` | Typically `PAGE_EXECUTE_READ` |
| `SND_INJ_STAGE_EXECUTED` | `snd_inj_classic_execute` | Remote thread created |

Debug builds expose human-readable stage names via `snd_inj_stage_to_string()`.

### Shellcode chain (`snd_inj_classic_shell`)

Linear pipeline on a single context:

```
open_target → alloc_remote → write_payload → set_protections → execute
```

Requires `inj_ctx.payload` pointing at raw shellcode before the chain runs.

### PE chain (`snd_inj_classic_pe`) — interleaved pipeline

PE injection **interleaves** loader and injection stages (not two independent full chains):

```
parse PE + compat check
allocate_and_copy_image (local bake)
open_target + alloc_remote
set execution_base = remote_base
apply_relocations (against remote base)
resolve_imports
write_payload (marshals local image → remote)
set_protections
execute (thread at remote entry RVA)
```

Notable differences from local reflective load:

- No `snd_ldr_pe_prepare_image` / `snd_ldr_pe_execute_image` on the remote path
- No TLS callbacks or DllMain in target — thread starts at entry RVA
- No per-section protection pass on remote path (classic chain writes then sets execute-read on whole region)
- `inj_ctx.remote_entry_point` is set to the **entry point VA** before `snd_inj_classic_execute`

---

## Stage validation errors

Both domains return **`SND_STATUS_INVALID_STAGE_SEQUENCE`** when:

- Current `stage` does not match the engine function's prerequisite
- DEBUG builds include formatted expected vs actual stage names in `status.context`

Allocation failure during section copy rolls the loader back (frees partial allocation, resets stage toward `SND_STAGE_PARSED`) — see loader engine implementation.

---

## Debug stage tracing

When `SND_ENABLE_DEBUG=ON`, chain and engine code emit transitions via `SND_DEBUG_PRINT`. Stage name strings in loader code use `SND_FALLBACK_STR("…")` so literal stage labels are stripped in SILENT builds.

---

## Adding a new domain technique

1. Choose context strategy: new loader context type **or** reuse `snd_inj_ctx_t`.
2. Define `typedef enum { … } your_stage_t` with monotonic pipeline order.
3. Implement engine functions that assert `ctx->stage == EXPECTED` before mutating state.
4. Provide `your_technique_chain(ctx)` that calls engine steps in order.
5. Document stage table alongside API reference.

---

## Related documentation

- [Dependency injection](dependency_injection.md)
- [Loaders techniques](../domains/loaders/techniques.md)
- [Injection techniques](../domains/injection/techniques.md)
- [Status system](status_system.md) — `SND_STATUS_INVALID_STAGE_SEQUENCE`
