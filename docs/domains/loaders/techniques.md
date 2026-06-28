# Loaders: Techniques

The loaders domain maps PE images into memory and prepares them for execution without the Windows loader. The first implemented technique is **Reflective PE Loading** under `loaders/reflective/`.

**Prerequisite reading:** [PE parser techniques](../../parsers/pe/techniques.md), [Dependency Injection](../../architecture/dependency_injection.md)

---

## Reflective PE Loading

Reflective loading maps and executes a PE (EXE or DLL) entirely in the **local** process without `LoadLibrary`. The image does not appear in the PEB module lists — no loader telemetry from standard module enumeration.

**Original research:** [Reflective DLL Injection](https://github.com/stephenfewer/ReflectiveDLLInjection) by Stephen Fewer (2008)

### Context and DI

Operations run through `snd_ldr_pe_ctx_t` with injected primitives:

```c
snd_ldr_pe_ctx_t ctx = {0};
ctx.raw_source = &payload_buf;
ctx.mem_api    = &snd_mem_sys;   // or snd_mem_win, snd_mem_nt
ctx.mod_api    = &snd_mod_nt;    // or snd_mod_win
```

Every memory and import operation calls `ctx->mem_api` / `ctx->mod_api` — never hardcoded Win32 APIs.

### Target block (`snd_pe_target_t`)

| Field | Role |
|---|---|
| `local_base` | Locally allocated image mapping (RW during fixups) |
| `execution_base` | Base used for relocation delta (defaults to `local_base`; set to remote base for injection bake) |
| `delta_offset` | `execution_base - ImageBase` |
| `entry_point` | Resolved entry (after `snd_ldr_pe_get_entry_point`) |
| `allocated_size` | `SizeOfImage` |

When `local_base != execution_base`, the image was prepared for a **remote** address (Classic PE injection). Local execute/detach paths refuse to run.

---

## Stage Machine (`snd_ldr_pe_stage_t`)

| Stage | Set by | Meaning |
|---|---|---|
| `SND_STAGE_UNINITIALIZED` | — | Empty context |
| `SND_STAGE_PARSED` | `snd_pe_parse` in chain/prepare | Raw PE validated |
| `SND_STAGE_MEM_ALLOCATED` | `snd_ldr_pe_allocate_and_copy_image` | Virtual memory reserved |
| `SND_STAGE_SECTIONS_MAPPED` | same (after section copy + re-parse mapped) | Sections copied, parser re-run with `is_mapped=TRUE` |
| `SND_STAGE_RELOCATED` | `snd_ldr_pe_apply_relocations` | Base relocations applied |
| `SND_STAGE_IMPORTS_RESOLVED` | `snd_ldr_pe_resolve_imports` | IAT patched |
| `SND_STAGE_READY_FOR_EXECUTION` | `snd_ldr_pe_apply_memory_protections` | Per-page protections applied |
| `SND_STAGE_EXECUTED` | `snd_ldr_pe_execute_image` | Entry point invoked |

Engine functions validate stage ordering and return `SND_STATUS_INVALID_STAGE_SEQUENCE` on violation.

---

## Reflective Pipeline (local execution)

### 1. Parse (`SND_STAGE_PARSED`)

`snd_pe_parse(raw_source, FALSE, &ctx.pe)` — raw file layout. Validates DOS/NT signatures and optional header magic.

### 2. Architecture check

`snd_ldr_pe_compatibility_check` — rejects cross-bitness payloads before allocation.

### 3. Allocate and copy (`SND_STAGE_MEM_ALLOCATED` → `SND_STAGE_SECTIONS_MAPPED`)

`snd_ldr_pe_allocate_and_copy_image`:

1. Tries `mem_api->alloc` at preferred `ImageBase`, falls back to OS-chosen base.
2. Copies headers and each section from file offsets to virtual addresses; zero-fills bss.
3. Re-parses the mapped buffer with `is_mapped=TRUE` so subsequent RVA lookups use virtual layout.

### 4. Apply relocations (`SND_STAGE_RELOCATED`)

`snd_ldr_pe_apply_relocations` sets `delta_offset = execution_base - ImageBase` and calls `snd_pe_apply_relocations`. No-op when delta is zero.

### 5. Resolve imports (`SND_STAGE_IMPORTS_RESOLVED`)

`snd_ldr_pe_resolve_imports` → `snd_pe_resolve_imports(local_base, mod_api, &pe)`. Requires mapped parser context.

### 6. Apply protections (`SND_STAGE_READY_FOR_EXECUTION`)

`snd_ldr_pe_apply_memory_protections` walks the image page-by-page, merging section characteristics per page, and applies protection **runs** via `mem_api->protect`. Headers default to read-only; section flags drive execute/read/write combinations. Avoids blanket `PAGE_EXECUTE_READWRITE` unless section flags demand it.

### 7. Execute (`SND_STAGE_EXECUTED`)

`snd_ldr_pe_execute_image`:

1. `snd_ldr_pe_get_entry_point`
2. `snd_ldr_pe_execute_tls_callbacks(DLL_PROCESS_ATTACH)`
3. **DLL:** call `DllMain(hinst, DLL_PROCESS_ATTACH, NULL)` — failure returns `SND_STATUS_DLL_INITIALIZATION_FAILED`
4. **EXE:** jump to entry point (does not return)

TLS and entry execution require `local_base == execution_base`.

### 8. Detach (optional cleanup)

`snd_ldr_pe_detach_image` — only when `stage == EXECUTED` and bases match locally:

- DLL: `DllMain(DETACH)`, TLS detach callbacks
- `snd_ldr_pe_free_mapped_image` — releases virtual memory, re-parses raw source

---

## Chain vs. Manual Staging

**Full chain** — `snd_ldr_pe_prepare_image` runs parse → compat → alloc/copy → reloc → imports → protections (no TLS, no entry).

**Execute** — `snd_ldr_pe_execute_image` runs TLS + entry (local only).

**Manual staging** — call individual `snd_ldr_pe_*` engine functions to pause between stages (sleep obfuscation, custom instrumentation).

### Post-load DLL exports (PoC pattern)

After `snd_ldr_pe_execute_image`, PoCs `loader_winapi` and `loader_nowinapi` optionally resolve a named export and invoke it through the FFI bridge:

```c
FARPROC fn = NULL;
snd_ldr_pe_get_proc_address(&ctx, export_name, &fn);
UINT_PTR ret = snd_ffi_execute((PVOID)(UINT_PTR)fn, argc, argv);
```

This runs **after** `DllMain(DLL_PROCESS_ATTACH)` has already fired during execute. Requires `ctx.stage >= SND_STAGE_READY_FOR_EXECUTION` (execute advances to `EXECUTED`; get_proc_address accepts `>= READY`).

---

## Required context fields

Before calling `snd_ldr_pe_prepare_image`:

| Field | Required |
|---|---|
| `raw_source` | Valid `snd_buffer_t` with PE bytes |
| `mem_api` | Non-NULL with `alloc`, `free`, `protect` |
| `mod_api` | Non-NULL with `load_library`, `get_proc_address` |

`target.execution_base` defaults to `local_base` inside `allocate_and_copy_image` when unset. Injection sets it to the remote base before relocations.

---

## Integration with Classic PE Injection

`snd_inj_classic_pe` uses a subset of the loader engine with a different ordering:

| Loader step | Used in injection? | Notes |
|---|---|---|
| Parse + alloc/copy | Yes | Local RW image |
| Relocations | Yes | `execution_base = remote_base` before apply |
| Imports | Yes | Local IAT fixup |
| Per-section protections | **No** | Injection uses flat RX on remote region |
| TLS + local execute | **No** | Remote thread starts at entry RVA |
| Write + remote protect + thread | Injection domain | Via `snd_inj_ctx_t` |

See [injection techniques](../injection/techniques.md).

---

## KnownDlls (bootstrapping, not a loader)

Retrieving clean `ntdll.dll` from `\KnownDlls` for syscall bootstrap is documented under [mapping primitives](../primitives/mapping/techniques.md). It is a **primitive** concern, not a loader technique.

Typical sequence before `snd_mem_sys` / `snd_proc_sys`:

```c
PVOID clean_ntdll = NULL;
snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &clean_ntdll);
snd_syscall_set_ntdll(clean_ntdll);
snd_syscall_strategy_set(snd_syscall_resolve_ssn_scan);
snd_syscall_strategy_add(snd_syscall_resolve_ssn_sort);
```

---

## Planned Loader Techniques

Future techniques will add new subdirectories under `loaders/` with their own context types and pipelines. Reflective loading remains the reference implementation for PE mapping semantics shared with injection bake paths.
