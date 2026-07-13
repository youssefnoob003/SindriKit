# Loaders: API Reference

Public loader API for the **Reflective PE** technique under `include/sindri/loaders/pe/`. Include `sindri/loaders.h` or `sindri/loaders/pe.h`.

KnownDlls mapping is documented under [mapping/object-manager](../primitives/mapping/api_reference.md) — not part of this API surface.

---

## Reflective Chain (`sindri/loaders/pe/chain.h`)

Primary entry points for end-to-end pe loading.

### `snd_ldr_pe_prepare_image`

Runs the full local preparation pipeline:

1. `snd_pe_parse(ctx->raw_source, FALSE, &ctx->pe)`
2. `snd_ldr_pe_compatibility_check`
3. `snd_ldr_pe_allocate_and_copy_image`
4. `snd_ldr_pe_apply_relocations`
5. `snd_ldr_pe_resolve_imports`
6. `snd_ldr_pe_apply_memory_protections`

Leaves context at `SND_STAGE_READY_FOR_EXECUTION`. Does **not** run TLS or invoke the entry point.

```c
snd_status_t snd_ldr_pe_prepare_image(snd_ldr_pe_ctx_t *ctx);
```

**Returns:** `SND_OK` or first failing stage status

**Source:** `src/loaders/pe/chain.c`

---

### `snd_ldr_pe_execute_image`

Executes a prepared image locally. Requires `SND_STAGE_READY_FOR_EXECUTION` and `local_base == execution_base`.

1. Resolves entry point
2. Runs TLS callbacks (`DLL_PROCESS_ATTACH`)
3. Invokes `DllMain` (DLL) or jumps to EXE entry

```c
snd_status_t snd_ldr_pe_execute_image(snd_ldr_pe_ctx_t *ctx);
```

**Returns:** `SND_OK`, `SND_STATUS_DLL_INITIALIZATION_FAILED`, `SND_STATUS_ENTRY_POINT_NOT_FOUND`, `SND_STATUS_UNSUPPORTED` (remote-prepared image)

---

### `snd_ldr_pe_detach_image`

Tears down a locally executed image. Requires `SND_STAGE_EXECUTED` and matching local/execution bases. Runs DLL detach + TLS detach, then frees mapped memory.

```c
void snd_ldr_pe_detach_image(snd_ldr_pe_ctx_t *ctx);
```

No-op if preconditions fail (including remote-prepared images).

---

## Reflective Engine (`sindri/loaders/pe/engine.h`)

### `snd_ldr_pe_ctx_t`

Central context for pe PE operations.

| Field | Type | Description |
|---|---|---|
| `raw_source` | `const snd_buffer_t *` | Raw on-disk PE bytes |
| `pe` | `snd_pe_parser_t` | Parsed PE context (raw then re-parsed mapped) |
| `target` | `snd_pe_target_t` | Allocation, delta, entry point state |
| `stage` | `snd_ldr_pe_stage_t` | Current pipeline stage |
| `mem_api` | `const snd_memory_api_t *` | Injected memory primitives |
| `mod_api` | `const snd_module_api_t *` | Injected module primitives |

---

### `snd_pe_target_t`

| Field | Type | Description |
|---|---|---|
| `local_base` | `LPVOID` | Local virtual mapping base |
| `execution_base` | `LPVOID` | Base for relocation delta (remote base when baking for injection) |
| `delta_offset` | `LONG_PTR` | `execution_base - ImageBase` |
| `entry_point` | `LPVOID` | Resolved entry (after get_entry_point) |
| `allocated_size` | `SIZE_T` | `SizeOfImage` allocation size |

---

### `snd_ldr_pe_stage_t`

| Constant | Value |
|---|---|
| `SND_STAGE_UNINITIALIZED` | 0 |
| `SND_STAGE_PARSED` | 1 |
| `SND_STAGE_MEM_ALLOCATED` | 2 |
| `SND_STAGE_SECTIONS_MAPPED` | 3 |
| `SND_STAGE_RELOCATED` | 4 |
| `SND_STAGE_IMPORTS_RESOLVED` | 5 |
| `SND_STAGE_READY_FOR_EXECUTION` | 6 |
| `SND_STAGE_EXECUTED` | 7 |

---

### Engine functions

| Function | Required stage | Description |
|---|---|---|
| `snd_ldr_pe_compatibility_check` | (after parse) | Host vs payload bitness |
| `snd_ldr_pe_allocate_and_copy_image` | `PARSED` | Alloc + section copy + re-parse mapped |
| `snd_ldr_pe_apply_relocations` | `SECTIONS_MAPPED` | Base relocation fixups |
| `snd_ldr_pe_resolve_imports` | `RELOCATED` | IAT patching via `mod_api` |
| `snd_ldr_pe_apply_memory_protections` | `IMPORTS_RESOLVED` | Per-page section protections |
| `snd_ldr_pe_get_entry_point` | `>= READY_FOR_EXECUTION` | Populates `target.entry_point` |
| `snd_ldr_pe_execute_tls_callbacks` | `>= READY_FOR_EXECUTION` | Invokes TLS callback array |
| `snd_ldr_pe_get_proc_address` | `>= READY_FOR_EXECUTION` | Export resolve via `snd_pe_get_export_address` |
| `snd_ldr_pe_free_mapped_image` | (any with `local_base`) | Frees mapping, resets toward `PARSED` |

**Source:** `src/loaders/pe/engine.c`

---

### `snd_ldr_pe_get_proc_address`

Resolves an export from the loaded image using the PE export parser. Forwarders use `mod_api->get_module_base` as resolver.

```c
snd_status_t snd_ldr_pe_get_proc_address(snd_ldr_pe_ctx_t *ctx, const char *func_name, FARPROC *func_addr_out);
```

---

### Export call macros

#### `SND_CALL_EXPORT`

Resolves and calls an export; discards return value.

```c
SND_CALL_EXPORT(ctx, name, signature, status_out, ...)
```

#### `SND_CALL_EXPORT_RET`

Resolves and calls an export; captures return value in `ret_out`.

```c
SND_CALL_EXPORT_RET(ctx, name, signature, status_out, ret_out, ...)
```

Both macros set `status_out` from `snd_ldr_pe_get_proc_address` and only invoke the function when resolution succeeds (`SND_SUCCEEDED(status_out)`).

---

## COFF Reflective Chain (`sindri/loaders/coff/chain.h`)

Primary entry points for end-to-end COFF object loading.

### `snd_ldr_coff_load`

Runs the full local COFF preparation pipeline:

1. `snd_coff_parse(ctx->raw_source, &ctx->coff)`
2. `snd_ldr_coff_allocate_and_copy_sections`
3. `snd_ldr_coff_resolve_symbols`
4. `snd_ldr_coff_apply_relocations`

Leaves context at `SND_COFF_STAGE_RELOCATED`. The COFF image is ready to execute.

```c
snd_status_t snd_ldr_coff_load(snd_ldr_coff_ctx_t *ctx);
```

**Returns:** `SND_OK` or first failing stage status
**Source:** `src/loaders/coff/chain.c`

---

### `snd_ldr_coff_execute_image`

Executes a loaded COFF (BOF) locally.

1. Locates the specified entry point by name (e.g., `"go"`)
2. Resolves the executable section containing the entry point
3. Invokes the entry point, passing the provided arguments buffer

```c
snd_status_t snd_ldr_coff_execute_image(snd_ldr_coff_ctx_t *ctx, const char *entry_point, void *args, int arg_len);
```

**Returns:** `SND_OK`, `SND_STATUS_COFF_SYMBOL_NOT_FOUND`
**Source:** `src/loaders/coff/chain.c`

---

## COFF Engine (`sindri/loaders/coff/engine.h`)

### `snd_ldr_coff_ctx_t`

Central context for COFF operations.

| Field | Type | Description |
|---|---|---|
| `raw_source` | `const snd_buffer_t *` | Raw on-disk COFF bytes |
| `coff` | `snd_coff_parser_t` | Parsed COFF context |
| `target` | `snd_coff_target_t` | Allocation state, section maps, symbol maps |
| `stage` | `snd_ldr_coff_stage_t` | Current pipeline stage |
| `mem_api` | `const snd_memory_api_t *` | Injected memory primitives |
| `mod_api` | `const snd_module_api_t *` | Injected module primitives |

### `snd_coff_target_t`

| Field | Type | Description |
|---|---|---|
| `local_base` | `LPVOID` | Local virtual mapping base |
| `execution_base` | `LPVOID` | Base for relocation delta (remote base when baking for injection) |
| `allocated_size` | `SIZE_T` | Total memory allocated for sections and metadata |
| `section_map` | `LPVOID *` | Map of 1-based section indices to memory pointers |
| `symbol_map` | `LPVOID *` | Map of symbol indices to resolved memory pointers |
| `iat_base` | `LPVOID` | Storage for resolved external function pointers (simulated IAT) |
| `trampolines_base` | `LPVOID` | x64 `JMP [RIP+0]` trampolines for distant calls |
| `bss_base` | `LPVOID` | Storage for uninitialized (`.bss`) symbols |

### `snd_ldr_coff_stage_t`

| Constant | Value |
|---|---|
| `SND_COFF_STAGE_UNINITIALIZED` | 0 |
| `SND_COFF_STAGE_PARSED` | 1 |
| `SND_COFF_STAGE_MEM_ALLOCATED` | 2 |
| `SND_COFF_STAGE_SECTIONS_MAPPED` | 3 |
| `SND_COFF_STAGE_SYMBOLS_RESOLVED` | 4 |
| `SND_COFF_STAGE_RELOCATED` | 5 |
| `SND_COFF_STAGE_READY_FOR_EXECUTION` | 6 |
| `SND_COFF_STAGE_EXECUTED` | 7 |

### Engine functions

| Function | Required stage | Description |
|---|---|---|
| `snd_ldr_coff_allocate_and_copy_sections` | `PARSED` | Allocates memory, copies sections, zeroes BSS |
| `snd_ldr_coff_resolve_symbols` | `SECTIONS_MAPPED` | Resolves external `MODULE$Func` via `mod_api` |
| `snd_ldr_coff_apply_relocations` | `SYMBOLS_RESOLVED` | Base relocation fixups per section |
| `snd_ldr_coff_free_mapped_image` | (any with `local_base`) | Frees mapping, resets toward `PARSED` |

**Source:** `src/loaders/coff/engine.c`

### Stage validation errors

Engine functions return `SND_STATUS_INVALID_STAGE_SEQUENCE` with context naming expected vs actual stage. Allocation failure during section copy rolls back to `SND_STAGE_PARSED` and frees `local_base`.

### `snd_ldr_pe_free_mapped_image` vs `snd_ldr_pe_detach_image`

| Function | When to use |
|---|---|
| `snd_ldr_pe_free_mapped_image` | Release virtual memory after prepare-only or failed load |
| `snd_ldr_pe_detach_image` | Full teardown after local execute: DllMain detach, TLS detach, then free |

`detach_image` is a no-op when `local_base != execution_base` (remote-prepared images from `snd_inj_classic_pe`).

---

## Usage Example (local pe load)

```c
snd_ldr_pe_ctx_t ctx = {0};
ctx.raw_source = &file_buf;
ctx.mem_api    = &snd_mem_win;
ctx.mod_api    = &snd_mod_win;

snd_status_t status = snd_ldr_pe_prepare_image(&ctx);
if (SND_FAILED(status)) goto cleanup;

status = snd_ldr_pe_execute_image(&ctx);
if (SND_FAILED(status)) goto cleanup;

snd_ldr_pe_detach_image(&ctx);
cleanup:
    snd_ldr_pe_free_mapped_image(&ctx);
```

---

## Related Documentation

- [Injection API](../injection/api_reference.md) — `snd_inj_classic_pe` reuses loader engine steps
- [PE parser API](../../parsers/pe/api_reference.md) — underlying parse/reloc/import primitives
- [Memory / module primitives](../primitives/memory/api_reference.md) — `mem_api` / `mod_api` backends
