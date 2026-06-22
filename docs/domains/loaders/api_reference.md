# Loaders: API Reference

This page documents the full public API surface of the loaders domain. Headers live under `include/sindri/loaders/`.

---

## Reflective Loader Chain (`sindri/loaders/reflective/chain.h`)

The chain functions are the primary entry points for reflective loading. They wrap the individual engine functions into a sequential pipeline, advancing the loader context's state machine automatically.

### `snd_prepare_reflective_image`

Executes the full allocation and fixup chain in one call:
1. Architecture compatibility check
2. Allocate virtual memory and copy PE sections
3. Apply base relocations
4. Resolve imports and patch the IAT
5. Apply per-section memory protections
6. Execute TLS callbacks

On success, the context is left in `SND_STAGE_READY_FOR_EXECUTION`.

| Parameter | Type | Description |
|---|---|---|
| `ctx` | `snd_loader_ctx_t*` | Initialized loader context with `raw_source`, `mem_api`, and `mod_api` set |

**Returns:** `snd_status_t` — `SND_OK` on success, or the status of the first failing stage.

---

### `snd_execute_reflective_image`

Resolves the entry point and calls it. Requires the context to be in `SND_STAGE_READY_FOR_EXECUTION`.

| Parameter | Type | Description |
|---|---|---|
| `ctx` | `snd_loader_ctx_t*` | Prepared loader context |

**Returns:** `snd_status_t` — `SND_OK` on success.

---

### `snd_detach_reflective_image`

Resets the loader context state. Does not free allocated virtual memory — call `snd_free_mapped_image` first if cleanup is required.

| Parameter | Type | Description |
|---|---|---|
| `ctx` | `snd_loader_ctx_t*` | Loader context to reset |

**Returns:** `void`

---

## Reflective Loader Engine (`sindri/loaders/reflective/engine.h`)

The engine functions expose each loading stage individually. These are utilized when fine-grained control over the pipeline is required — for example, to pause between stages for sleep obfuscation, or to inspect context state mid-operation.

### `snd_pe_target_t`

Structure tracking the state of the allocated target region during the loading process.

| Field | Type | Description |
|---|---|---|
| `virtual_base` | `LPVOID` | Allocated virtual memory base, populated after `SND_STAGE_MEM_ALLOCATED` |
| `delta_offset` | `LONG_PTR` | Relocation delta (actual base minus preferred `ImageBase`) |
| `entry_point` | `LPVOID` | Resolved entry point address |
| `allocated_size` | `SIZE_T` | Total size of the virtual allocation |

---

### `snd_loader_ctx_t`

The central context structure for a reflective load operation.

| Field | Type | Description |
|---|---|---|
| `raw_source` | `const snd_buffer_t*` | Raw payload buffer (the on-disk PE bytes) |
| `pe` | `snd_pe_parser_t` | Parsed PE context, populated after stage `SND_STAGE_PARSED` |
| `target` | `snd_pe_target_t` | Target state block containing allocated memory and delta data |
| `stage` | `snd_loader_stage_t` | Current state machine position |
| `mem_api` | `const snd_memory_api_t*` | Injected memory primitives |
| `mod_api` | `const snd_module_api_t*` | Injected module primitives |

---

### `snd_compatibility_check`

Validates that the payload's architecture matches the host process bitness.

| Parameter | Type | Description |
|---|---|---|
| `ctx` | `snd_loader_ctx_t*` | Loader context with a populated `pe` field |

**Returns:** `snd_status_t` — `SND_OK` on match, `SND_STATUS_ARCH_MISMATCH` otherwise.

---

### `snd_allocate_and_copy_image`

Allocates `SizeOfImage` bytes via `ctx->mem_api->alloc` and copies PE sections from their raw file offsets to their virtual addresses. Populates `ctx->target.virtual_base` and `ctx->target.allocated_size`.

| Parameter | Type | Description |
|---|---|---|
| `ctx` | `snd_loader_ctx_t*` | Loader context with a populated `pe` field |

**Returns:** `snd_status_t` — `SND_OK` on success, or an allocation/copy error.

---

### `snd_apply_relocations`

Applies base relocation fixups. Computes the delta between the actual allocated base and the PE's preferred `ImageBase`, then patches each pointer-sized field in the `.reloc` blocks.

| Parameter | Type | Description |
|---|---|---|
| `ctx` | `snd_loader_ctx_t*` | Loader context with `target.virtual_base` populated |

**Returns:** `snd_status_t` — `SND_OK` on success, or a relocation error (e.g., `SND_STATUS_RELOCATION_PATCH_OUT_OF_RANGE`).

---

### `snd_resolve_imports`

Walks the Import Descriptor table. For each imported DLL, calls `ctx->mod_api->load_library`. For each thunk, calls `ctx->mod_api->get_proc_address` (by name) or resolves by ordinal. Writes resolved addresses into the IAT.

| Parameter | Type | Description |
|---|---|---|
| `ctx` | `snd_loader_ctx_t*` | Loader context with sections mapped and relocated |

**Returns:** `snd_status_t` — `SND_OK` on success, or an import resolution error.

---

### `snd_apply_memory_protections`

Iterates sections and sets per-section page protections via `ctx->mem_api->protect`, based on each section's `Characteristics` flags (`MEM_EXECUTE`, `MEM_READ`, `MEM_WRITE`).

| Parameter | Type | Description |
|---|---|---|
| `ctx` | `snd_loader_ctx_t*` | Loader context with sections mapped and imports resolved |

**Returns:** `snd_status_t` — `SND_OK` on success, `SND_STATUS_PROTECTION_UPDATE_FAILED` on error.

---

### `snd_execute_tls_callbacks`

Locates the TLS data directory and calls each `PIMAGE_TLS_CALLBACK` in the callback array.

| Parameter | Type | Description |
|---|---|---|
| `ctx` | `snd_loader_ctx_t*` | Loader context with protections applied |
| `reason` | `DWORD` | Callback reason code (e.g., `DLL_PROCESS_ATTACH`) |

**Returns:** `void`

---

### `snd_get_entry_point`

Resolves the entry point address from `OptionalHeader.AddressOfEntryPoint` and stores it in `ctx->target.entry_point`.

| Parameter | Type | Description |
|---|---|---|
| `ctx` | `snd_loader_ctx_t*` | Loader context |

**Returns:** `snd_status_t` — `SND_OK` on success, `SND_STATUS_ENTRY_POINT_NOT_FOUND` if invalid.

---

### `snd_get_proc_address`

Resolves an exported symbol from the reflectively loaded image by walking its export table. Does not require the image to be registered in the PEB.

| Parameter | Type | Description |
|---|---|---|
| `ctx` | `snd_loader_ctx_t*` | Loader context of the loaded image |
| `func_name` | `const char*` | Plaintext name of the target export |
| `func_addr_out` | `FARPROC*` | Receives the resolved export address |

**Returns:** `snd_status_t` — `SND_OK` on success, `SND_STATUS_EXPORT_NOT_FOUND` if not found.

---

### `snd_free_mapped_image`

Frees the virtual memory region allocated for the mapped image via `ctx->mem_api->free`.

| Parameter | Type | Description |
|---|---|---|
| `ctx` | `snd_loader_ctx_t*` | Loader context with a valid `target.virtual_base` |

**Returns:** `void`

---

### `SND_CALL_EXPORT`

Resolves and calls a named export from the loaded image. The return value of the export is discarded.

```c
SND_CALL_EXPORT(ctx, name, signature, status_out, ...)
```

| Parameter | Description |
|---|---|
| `ctx` | Pointer to the initialized loader context |
| `name` | Plaintext string name of the target export |
| `signature` | Function pointer type to cast the export to (e.g., `void (*)(void)`) |
| `status_out` | A `snd_status_t` variable that receives the resolution result |
| `...` | Arguments to pass to the export |

---

### `SND_CALL_EXPORT_RET`

Resolves and calls a named export, capturing its return value.

```c
SND_CALL_EXPORT_RET(ctx, name, signature, status_out, ret_out, ...)
```

| Parameter | Description |
|---|---|
| `ctx` | Pointer to the initialized loader context |
| `name` | Plaintext string name of the target export |
| `signature` | Function pointer type to cast the export to |
| `status_out` | A `snd_status_t` variable that receives the resolution result |
| `ret_out` | Variable to store the export's return value |
| `...` | Arguments to pass to the export |

---

## KnownDlls Loader (`sindri/loaders/knowndlls/knowndlls.h`)

### `SND_TARGET_KNOWNDLLS_DIR`

**Value:** `L"\\KnownDlls\\"` (x64) or `L"\\KnownDlls32\\"` (x86)

The architecture-dependent Object Manager directory path where pristine system DLL section objects are cached by `smss.exe`.

---

### `snd_knowndlls_config_t`

Configuration structure for the KnownDlls loader mapping technique. Two pre-built instances are exported globally:

- `snd_knowndlls_win`: Uses Win32 implementations (e.g., `NtOpenSection` via `GetProcAddress`).
- `snd_knowndlls_native`: Uses direct syscall implementations.

---

### `snd_map_knowndll`

Opens the named DLL's section object from the `\KnownDlls` Object Manager directory and maps a view of it into the current process.

| Parameter | Type | Description |
|---|---|---|
| `config` | `const snd_knowndlls_config_t*` | Pointer to the injected API configuration (`&snd_knowndlls_win` or `&snd_knowndlls_native`) |
| `dll_name` | `const wchar_t*` | Exact DLL name as it appears in `\KnownDlls` (e.g., `L"ntdll.dll"`) |
| `out_base_address` | `PVOID*` | Receives the base address of the mapped section |

**Returns:** `snd_status_t` — `SND_OK` on success.
