# SindriKit API Reference

## Table of Contents

- [Common](#common)
- [Macros (`sindri/common/macros.h`)](#macros-sindricommonmacrosh)
- [Memory (`sindri/common/memory.h`)](#memory-sindricommonmemoryh)
  - [`SND_PTR_ADD(base, offset)`](#snd_ptr_addbase-offset)
  - [`snd_memory_bounds_check`](#snd_memory_bounds_check)
  - [`snd_memory_ptr_bounds_check`](#snd_memory_ptr_bounds_check)
  - [`snd_memzero`](#snd_memzero)
  - [`snd_memcpy`](#snd_memcpy)
- [String (`sindri/common/string.h`)](#string-sindricommonstringh)
  - [ASCII](#ascii)
  - [Wide (UTF-16)](#wide-utf-16)
- [Buffer (`sindri/common/buffer.h`)](#buffer-sindricommonbufferh)
  - [`snd_buffer_t`](#snd_buffer_t)
  - [`snd_free_cb`](#snd_free_cb)
  - [`snd_buffer_init`](#snd_buffer_init)
  - [`snd_buffer_free`](#snd_buffer_free)
  - [`snd_buffer_bounds_check`](#snd_buffer_bounds_check)
- [Hashing (`sindri/common/hash.h`)](#hashing-sindricommonhashh)
  - [`snd_hash`](#snd_hash)
  - [`snd_hash_lower`](#snd_hash_lower)
  - [`snd_hash_wide_lower`](#snd_hash_wide_lower)
- [Status (`sindri/status.h`)](#status-sindristatush)
  - [`snd_status_t`](#snd_status_t)
  - [Convenience macros](#convenience-macros)
  - [`SND_MAX_CTX_LEN`](#snd_max_ctx_len)
  - [Core generic status codes (`sindri/status/core.h`)](#core-generic-status-codes-sindristatuscoreh)
  - [`snd_status_to_string`](#snd_status_to_string)
  - [`snd_status_print`](#snd_status_print)
- [Debug (`sindri/common/debug.h`)](#debug-sindricommondebugh)
  - [`SND_DEBUG_PRINT(fmt, …)`](#snd_debug_printfmt)
  - [`SND_FDEBUG_PRINT(stream, fmt, …)`](#snd_fdebug_printstream-fmt)
  - [`SND_FALLBACK_STR(debug_str)`](#snd_fallback_strdebug_str)
  - [`snd_dump_hex`](#snd_dump_hex)
  - [`SND_DEBUG_MAX_LEN`](#snd_debug_max_len)
- [File primitives (`sindri/primitives/files.h`)](#file-primitives-sindriprimitivesfilesh)
  - [`snd_file_api_t` / `snd_file_win` / `snd_file_nt` / `snd_file_sys`](#snd_file_api_t-snd_file_win-snd_file_nt-snd_file_sys)
- [CRT manifest (`src/common/crt_manifest.c`)](#crt-manifest-srccommoncrt_manifestc)
- [Internal NT layouts (`include/sindri/internal/nt/`)](#internal-nt-layouts-includesindriinternalnt)
  - [`base.h`](#baseh)
  - [`api.h`](#apih)
  - [`peb.h`](#pebh)
- [Related documentation](#related-documentation)
- [Evasion](#evasion)
- [Injection](#injection)
- [Shared Context (`sindri/injection/context.h`)](#shared-context-sindriinjectioncontexth)
  - [`snd_inj_ctx_t`](#snd_inj_ctx_t)
  - [`snd_inj_stage_t`](#snd_inj_stage_t)
  - [`snd_inj_stage_to_string`](#snd_inj_stage_to_string)
  - [`snd_inj_cleanup`](#snd_inj_cleanup)
- [Classic Engine (`sindri/injection/classic/engine.h`)](#classic-engine-sindriinjectionclassicengineh)
  - [`snd_inj_classic_open_target`](#snd_inj_classic_open_target)
  - [`snd_inj_classic_alloc_remote`](#snd_inj_classic_alloc_remote)
  - [`snd_inj_classic_write_payload`](#snd_inj_classic_write_payload)
  - [`snd_inj_classic_set_protections`](#snd_inj_classic_set_protections)
  - [`snd_inj_classic_execute`](#snd_inj_classic_execute)
- [Classic Chain (`sindri/injection/classic/chain.h`)](#classic-chain-sindriinjectionclassicchainh)
  - [`snd_inj_classic_shell`](#snd_inj_classic_shell)
  - [`snd_inj_classic_pe`](#snd_inj_classic_pe)
  - [`snd_inj_classic_coff`](#snd_inj_classic_coff)
- [APC Engine (`sindri/injection/apc/engine.h`)](#apc-engine-sindriinjectionapcengineh)
- [APC Chain (`sindri/injection/apc/chain.h`)](#apc-chain-sindriinjectionapcchainh)
  - [`snd_inj_apc_shell`](#snd_inj_apc_shell)
  - [`snd_inj_apc_pe`](#snd_inj_apc_pe)
  - [`snd_inj_apc_coff`](#snd_inj_apc_coff)
- [Related Documentation](#related-documentation)
- [Loaders](#loaders)
- [Reflective Chain (`sindri/loaders/pe/chain.h`)](#reflective-chain-sindriloaderspechainh)
  - [`snd_ldr_pe_prepare_image`](#snd_ldr_pe_prepare_image)
  - [`snd_ldr_pe_execute_image`](#snd_ldr_pe_execute_image)
  - [`snd_ldr_pe_detach_image`](#snd_ldr_pe_detach_image)
- [Reflective Engine (`sindri/loaders/pe/engine.h`)](#reflective-engine-sindriloaderspeengineh)
  - [`snd_ldr_pe_ctx_t`](#snd_ldr_pe_ctx_t)
  - [`snd_pe_target_t`](#snd_pe_target_t)
  - [`snd_ldr_pe_stage_t`](#snd_ldr_pe_stage_t)
  - [Engine functions](#engine-functions)
  - [`snd_ldr_pe_get_proc_address`](#snd_ldr_pe_get_proc_address)
  - [Export call macros](#export-call-macros)
- [COFF Reflective Chain (`sindri/loaders/coff/chain.h`)](#coff-reflective-chain-sindriloaderscoffchainh)
  - [`snd_ldr_coff_load`](#snd_ldr_coff_load)
  - [`snd_ldr_coff_execute_image`](#snd_ldr_coff_execute_image)
- [COFF Engine (`sindri/loaders/coff/engine.h`)](#coff-engine-sindriloaderscoffengineh)
  - [`snd_ldr_coff_ctx_t`](#snd_ldr_coff_ctx_t)
  - [`snd_coff_target_t`](#snd_coff_target_t)
  - [`snd_ldr_coff_stage_t`](#snd_ldr_coff_stage_t)
  - [Engine functions](#engine-functions)
  - [Stage validation errors](#stage-validation-errors)
  - [`snd_ldr_pe_free_mapped_image` vs `snd_ldr_pe_detach_image`](#snd_ldr_pe_free_mapped_image-vs-snd_ldr_pe_detach_image)
- [Usage Example (local pe load)](#usage-example-local-pe-load)
- [Related Documentation](#related-documentation)
- [Parsers: Coff](#parsers-coff)
- [Core Parser (`sindri/parsers/coff/parser.h`)](#core-parser-sindriparserscoffparserh)
  - [`snd_coff_parse`](#snd_coff_parse)
  - [`snd_coff_parser_t`](#snd_coff_parser_t)
- [Utilities (`sindri/parsers/coff/utils.h`)](#utilities-sindriparserscoffutilsh)
  - [`snd_coff_get_symbol_name`](#snd_coff_get_symbol_name)
  - [`snd_coff_get_section_name`](#snd_coff_get_section_name)
  - [`snd_coff_raw_to_ptr`](#snd_coff_raw_to_ptr)
  - [`snd_coff_write_jmp_trampoline`](#snd_coff_write_jmp_trampoline)
- [Symbols (`sindri/parsers/coff/symbols.h`)](#symbols-sindriparserscoffsymbolsh)
  - [Constants and Types](#constants-and-types)
  - [`snd_coff_get_symbol_by_index`](#snd_coff_get_symbol_by_index)
  - [`snd_coff_find_symbol_by_name`](#snd_coff_find_symbol_by_name)
  - [`snd_coff_decode_symbol`](#snd_coff_decode_symbol)
- [Relocations (`sindri/parsers/coff/relocations.h`)](#relocations-sindriparserscoffrelocationsh)
  - [`snd_coff_get_relocations`](#snd_coff_get_relocations)
  - [`snd_coff_get_relocation_patch_size`](#snd_coff_get_relocation_patch_size)
- [Parsers: Env](#parsers-env)
- [PEB Module Resolution (`sindri/parsers/env/peb.h`)](#peb-module-resolution-sindriparsersenvpebh)
  - [`snd_peb_get_local`](#snd_peb_get_local)
  - [`snd_peb_get_module_base`](#snd_peb_get_module_base)
  - [`snd_peb_get_module_base_hash`](#snd_peb_get_module_base_hash)
- [NTDLL State Management (`sindri/parsers/env/ntdll.h`)](#ntdll-state-management-sindriparsersenvntdllh)
  - [`snd_ntdll_entry_t`](#snd_ntdll_entry_t)
  - [`snd_ntdll_set_clean`](#snd_ntdll_set_clean)
  - [`snd_ntdll_get_active_parser` / `snd_ntdll_get_active_base`](#snd_ntdll_get_active_parser-snd_ntdll_get_active_base)
  - [`snd_ntdll_get_clean_parser` / `snd_ntdll_get_clean_base`](#snd_ntdll_get_clean_parser-snd_ntdll_get_clean_base)
  - [`snd_ntdll_get_active_export` / `snd_ntdll_get_clean_export`](#snd_ntdll_get_active_export-snd_ntdll_get_clean_export)
- [NT Structures (`sindri/internal/nt/peb.h`)](#nt-structures-sindriinternalntpebh)
- [Usage as Export Forwarder Resolver](#usage-as-export-forwarder-resolver)
- [Status Codes](#status-codes)
- [Related Documentation](#related-documentation)
- [Parsers: Pe](#parsers-pe)
- [Core Parser (`sindri/parsers/pe/parser.h`)](#core-parser-sindriparserspeparserh)
  - [`snd_pe_parse`](#snd_pe_parse)
  - [`snd_pe_parser_t`](#snd_pe_parser_t)
  - [Macros](#macros)
- [Utilities (`sindri/parsers/pe/utils.h`)](#utilities-sindriparserspeutilsh)
  - [`snd_pe_get_directory`](#snd_pe_get_directory)
  - [`snd_pe_rva_to_ptr`](#snd_pe_rva_to_ptr)
  - [`snd_pe_get_entry_point`](#snd_pe_get_entry_point)
  - [`snd_pe_get_tls_callbacks`](#snd_pe_get_tls_callbacks)
  - [`snd_pe_get_page_protection_flags`](#snd_pe_get_page_protection_flags)
- [Export Resolution (`sindri/parsers/pe/exports.h`)](#export-resolution-sindriparserspeexportsh)
  - [`snd_pe_get_export_address`](#snd_pe_get_export_address)
  - [`snd_pe_get_export_address_hash`](#snd_pe_get_export_address_hash)
  - [`snd_pe_enumerate_exports`](#snd_pe_enumerate_exports)
  - [`SND_FWD_MAX_DEPTH`](#snd_fwd_max_depth)
- [Import Parsing (`sindri/parsers/pe/imports.h`)](#import-parsing-sindriparserspeimportsh)
  - [`snd_pe_import_thunk_t`](#snd_pe_import_thunk_t)
  - [`snd_pe_get_import_descriptor`](#snd_pe_get_import_descriptor)
  - [`snd_pe_get_import_name`](#snd_pe_get_import_name)
  - [`snd_pe_get_import_thunk`](#snd_pe_get_import_thunk)
  - [Ordinal Macros](#ordinal-macros)
- [Base Relocations (`sindri/parsers/pe/relocations.h`)](#base-relocations-sindriparsersperelocationsh)
  - [`snd_pe_reloc_entry_t`](#snd_pe_reloc_entry_t)
  - [`snd_pe_get_reloc_block`](#snd_pe_get_reloc_block)
  - [`snd_pe_get_reloc_entry`](#snd_pe_get_reloc_entry)
- [Section Helpers (`sindri/parsers/pe/section.h`)](#section-helpers-sindriparserspesectionh)
- [Primitives: Execution](#primitives-execution)
- [Dynamic Invocation (`sindri/primitives/ffi.h`)](#dynamic-invocation-sindriprimitivesffih)
  - [`snd_ffi_execute`](#snd_ffi_execute)
- [Heaven's Gate (`sindri/primitives/heavens_gate.h`)](#heavens-gate-sindriprimitivesheavens_gateh)
  - [`snd_is_wow64`](#snd_is_wow64)
  - [`snd_hg_execute_64`](#snd_hg_execute_64)
- [Internal ASM symbols (not public API)](#internal-asm-symbols-not-public-api)
- [Primitives: Mapping](#primitives-mapping)
- [Mapping API Instances (`sindri/primitives/mapping.h`)](#mapping-api-instances-sindriprimitivesmappingh)
- [The Interface (`sindri/primitives/os_api.h`)](#the-interface-sindriprimitivesos_apih)
  - [`snd_mapping_api_t`](#snd_mapping_api_t)
- [Object Manager Helper (`sindri/primitives/object_manager.h`)](#object-manager-helper-sindriprimitivesobject_managerh)
  - [`snd_om_knowndll_map`](#snd_om_knowndll_map)
- [Primitives: Memory](#primitives-memory)
- [Memory API Instances (`sindri/primitives/memory.h`)](#memory-api-instances-sindriprimitivesmemoryh)
- [The Interface (`sindri/primitives/os_api.h`)](#the-interface-sindriprimitivesos_apih)
  - [`snd_memory_api_t`](#snd_memory_api_t)
- [Dependency Injection](#dependency-injection)
- [Primitives: Modules](#primitives-modules)
- [Module API Instances (`sindri/primitives/modules.h`)](#module-api-instances-sindriprimitivesmodulesh)
- [The Interface (`sindri/primitives/os_api.h`)](#the-interface-sindriprimitivesos_apih)
  - [`snd_module_api_t`](#snd_module_api_t)
- [Related Parsers (`sindri/parsers/env/peb.h`)](#related-parsers-sindriparsersenvpebh)
- [Dependency Injection](#dependency-injection)
- [Primitives: Process](#primitives-process)
- [Process API Instances (`sindri/primitives/process.h`)](#process-api-instances-sindriprimitivesprocessh)
- [The Interface (`sindri/primitives/os_api.h`)](#the-interface-sindriprimitivesos_apih)
  - [`snd_process_api_t`](#snd_process_api_t)
  - [Customizing process creation (`sindri/primitives/process.h`)](#customizing-process-creation-sindriprimitivesprocessh)
  - [`snd_thread_api_t`](#snd_thread_api_t)
- [Injection Context (`sindri/injection/context.h`)](#injection-context-sindriinjectioncontexth)
- [Primitives: Syscalls](#primitives-syscalls)
- [Constants](#constants)
- [Core Data Structures](#core-data-structures)
  - [`snd_syscall_entry_t`](#snd_syscall_entry_t)
  - [`snd_syscall_args_t`](#snd_syscall_args_t)
- [Pipeline Configuration](#pipeline-configuration)
  - [`snd_syscall_resolver_t`](#snd_syscall_resolver_t)
  - [`snd_syscall_set_resolver`](#snd_syscall_set_resolver)
  - [`snd_syscall_add_resolver`](#snd_syscall_add_resolver)
- [Built-in Resolvers](#built-in-resolvers)
  - [`snd_syscall_resolve_ssn_scan`](#snd_syscall_resolve_ssn_scan)
  - [`snd_syscall_resolve_ssn_sort`](#snd_syscall_resolve_ssn_sort)
  - [`snd_syscall_set_invoker`](#snd_syscall_set_invoker)
  - [`snd_syscall_set_gadget_finder`](#snd_syscall_set_gadget_finder)
  - [`snd_syscall_set_spoof_finder`](#snd_syscall_set_spoof_finder)
- [Resolution & Execution](#resolution-execution)
  - [`snd_syscall_resolve`](#snd_syscall_resolve)
  - [`snd_syscall_direct_invoke_asm`](#snd_syscall_direct_invoke_asm)
  - [`snd_syscall_indirect_invoke_asm`](#snd_syscall_indirect_invoke_asm)
  - [`snd_syscall_spoofed_invoke_asm`](#snd_syscall_spoofed_invoke_asm)
- [Compile-Time Defaults (`SND_USE_DEFAULTS`)](#compile-time-defaults-snd_use_defaults)
- [Typical bootstrap (PoC pattern)](#typical-bootstrap-poc-pattern)
- [Related documentation](#related-documentation)

---


## Common


Public API under `include/sindri/common/`. Include `sindri/common.h` for the full surface.

Implementation sources: `src/common/` (`buffer.c`, `hash.c`, `debug.c`, `crt_manifest.c`) and `src/status/` (`status.c`, `generic.c`, `cli.c`, `file.c`).

---

## Macros (`sindri/common/macros.h`)

| Macro | Description |
|---|---|
| `SND_BEGIN_EXTERN_C` / `SND_END_EXTERN_C` | C++ `extern "C"` linkage wrappers |
| `SND_FORCE_INLINE` | `static __forceinline` (MSVC) or `always_inline` (GCC/Clang) |

---

## Memory (`sindri/common/memory.h`)

Header-only CRT replacements and bounds helpers.

### `SND_PTR_ADD(base, offset)`

Adds a byte offset to a base pointer (casts through `unsigned char *`).

### `snd_memory_bounds_check`

```c
SND_FORCE_INLINE int snd_memory_bounds_check(size_t total_size, size_t offset, size_t size);
```

Returns `1` if `[offset, offset+size)` fits within `[0, total_size)`.

### `snd_memory_ptr_bounds_check`

```c
SND_FORCE_INLINE int snd_memory_ptr_bounds_check(
    const void *base, size_t total_size, const void *ptr, size_t size);
```

Validates that `[ptr, ptr+size)` lies within `[base, base+total_size)`. Returns `0` if `base` or `ptr` is NULL, or if `ptr < base`.

### `snd_memzero`

Zeroes `size` bytes at `dest` using a `volatile` loop (prevents compiler elision). Replaces `memset`.

### `snd_memcpy`

Copies `count` bytes from `src` to `dest`. Replaces `memcpy`.

---

## String (`sindri/common/string.h`)

Header-only bounded string operations.

### ASCII

| Function | Replaces | Notes |
|---|---|---|
| `snd_strnlen(str, max_len)` | `strnlen` | |
| `snd_strncpy(dest, dest_size, src, max_src_len)` | `strncpy` | Always null-terminates |
| `snd_strncat(dest, dest_size, src, max_src_len)` | `strncat` | Bounded append |
| `snd_strnchr(str, c, max_len)` | `strchr` | Bounded search |
| `snd_strncmp(s1, s2, max_len)` | `strncmp` | Returns `<0 / 0 / >0` |
| `snd_strnicmp(s1, s2, max_len)` | `strnicmp` | Case-insensitive bounded comparison |
| `snd_atou32_bounded(str, max_len, out_val)` | `strtoul` | ASCII decimal to uint32_t |

### Wide (UTF-16)

| Function | Replaces | Notes |
|---|---|---|
| `snd_wcsnlen(str, max_len)` | `wcsnlen` | |
| `snd_wcsnicmp(s1, s2, n)` | `wcsnicmp` | Case-insensitive |
| `snd_wcsncpy(dest, dest_size, src, max_src_len)` | `wcsncpy` | |
| `snd_wcsncat(dest, dest_size, src, max_src_len)` | `wcsncat` | |
| `snd_ascii_to_wide(dest, dest_size, src, max_src_len)` | `mbstowcs` | Latin ASCII → wide |
| `snd_wcsn_env_size(env, max_chars)` | — | Computes double-null terminated block size |

Used by PEB walking, export forwarders, and `LdrLoadDll` path construction.

---

## Buffer (`sindri/common/buffer.h`)

### `snd_buffer_t`

```c
typedef struct snd_buffer_s {
    void       *data;
    size_t      size;
    snd_free_cb free_routine;
} snd_buffer_t;
```

### `snd_free_cb`

```c
typedef void (*snd_free_cb)(snd_buffer_t *buf);
```

### `snd_buffer_init`

Initializes tracking metadata without allocating memory.

```c
void snd_buffer_init(snd_buffer_t *buf, void *data, size_t size, snd_free_cb free_routine);
```

### `snd_buffer_free`

Invokes `free_routine` if set, then zeroes the struct.

```c
void snd_buffer_free(snd_buffer_t *buf);
```

**Source:** `src/common/buffer.c`

### `snd_buffer_bounds_check`

```c
SND_FORCE_INLINE int snd_buffer_bounds_check(const snd_buffer_t *buf, size_t offset, size_t size);
```

Returns `0` if `buf` is NULL, `buf->data` is NULL, or `buf->size` is zero.

---

## Hashing (`sindri/common/hash.h`)

Runtime hashing uses the algorithm selected at CMake configure time.

### `snd_hash`

```c
uint32_t snd_hash(const char *str);
```

Case-sensitive. Export names from the EAT.

### `snd_hash_lower`

```c
uint32_t snd_hash_lower(const char *str);
```

Lowercases ASCII before hashing. Import DLL names.

### `snd_hash_wide_lower`

```c
uint32_t snd_hash_wide_lower(const wchar_t *str);
```

Lowercases wide string before hashing. PEB module names.

**Source:** `src/common/hash.c`

---

## Status (`sindri/status.h`)

For a complete, auto-generated list of all status codes, see [Status Codes](status_codes.md).


### `snd_status_t`

| Field | Debug (`SND_DEBUG=1`) | Release |
|---|---|---|
| `code` | `snd_status_code_t` | same |
| `os_error` | `int` | same |
| `file` | source file | *(omitted)* |
| `line` | line number | *(omitted)* |
| `context` | 128-byte formatted string | *(omitted)* |

### Convenience macros

| Macro | Description |
|---|---|
| `SND_SUCCEEDED(x)` | `(x.code == SND_SUCCESS)` |
| `SND_FAILED(x)` | `(x.code != SND_SUCCESS)` |
| `SND_OK` | Success status |
| `SND_ERR(code)` | Framework error |
| `SND_ERR_CTX(code, fmt, …)` | Error with formatted context (debug only) |
| `SND_ERR_W32(code)` | Error + `GetLastError()` |
| `SND_ERR_W32_CTX(code, fmt, …)` | W32 + context |
| `SND_ERR_NT(code, nt_status)` | Error + NTSTATUS |
| `SND_ERR_NT_CTX(code, nt_status, fmt, …)` | NT + context |
| `SND_CHECK_NULL(p1[, p2, …])` | Returns `SND_STATUS_NULL_POINTER` if any of up to 5 pointer args is NULL |
| `SND_TRY(expr)` | Evaluates `expr` and returns immediately on failure |

### `SND_MAX_CTX_LEN`

**Value:** `128` — max formatted context string length.

### Core generic status codes (`sindri/status/core.h`)

| Code | Facility | Description |
|---|---|---|
| `SND_SUCCESS` | Generic | No error |
| `SND_ERROR_GENERIC` | Generic | Unclassified error |
| `SND_STATUS_NULL_POINTER` | Generic | A required pointer argument is NULL |
| `SND_STATUS_INVALID_PARAMETERS_COMBINATION` | Generic | Contradictory parameter combination |
| `SND_STATUS_UNSUPPORTED` | Generic | Feature not supported for this configuration |
| `SND_STATUS_ARCH_MISMATCH` | Generic | Host vs payload bitness mismatch |
| `SND_STATUS_TOO_MANY_ARGUMENTS` | Generic | Argument count exceeded limit (e.g. FFI > 6) |
| `SND_STATUS_MISSING_COMMAND_LINE_ARGS` | CLI | Required CLI argument missing |
| `SND_STATUS_INVALID_COMMAND_LINE_ARG` | CLI | Unrecognized or malformed CLI argument |
| `SND_STATUS_FILE_INVALID_PATH` | File | Path string is NULL or empty |
| `SND_STATUS_FILE_CREATE_FAILED` | File | File open/create call failed |
| `SND_STATUS_FILE_SIZE_QUERY_FAILED` | File | Failed to query file size |
| `SND_STATUS_FILE_TOO_LARGE` | File | File exceeds maximum size limit |
| `SND_STATUS_FILE_TOO_SMALL` | File | File is too small to be valid |
| `SND_STATUS_FILE_READ_FAILED` | File | Read operation failed |
| `SND_STATUS_FILE_ALLOC_FAILED` | File | Buffer allocation for file content failed |
| `SND_STATUS_INVALID_STAGE` | Context machines | Stage precondition not met |
| `SND_STATUS_CORRUPTED_STAGE` | Context machines | Internal state is inconsistent |

### `snd_status_to_string`

```c
const char *snd_status_to_string(snd_status_t status);
```

Human-readable description. Returns empty string when `SND_DEBUG=0`.

### `snd_status_print`

```c
void snd_status_print(snd_status_t status);
```

Prints code, context, OS error, and file/line via `SND_DEBUG_PRINT`. No-op body when `SND_DEBUG=0`.

**Source:** `src/status/status.c`, `src/status/generic.c`, `src/status/cli.c`, `src/status/file.c`

---

## Debug (`sindri/common/debug.h`)

Controlled by CMake `SND_ENABLE_DEBUG` → `SND_DEBUG`.

### `SND_DEBUG_PRINT(fmt, …)`

Formatted debug output. No-op when `SND_DEBUG=0`.

### `SND_FDEBUG_PRINT(stream, fmt, …)`

Stream-targeted output when `SND_USE_PRINTF=1`; otherwise same as `SND_DEBUG_PRINT`.

### `SND_FALLBACK_STR(debug_str)`

Returns `debug_str` when `SND_DEBUG=1`, `""` when `SND_DEBUG=0`. Used for stage name strings in state machines.

### `snd_dump_hex`

```c
void snd_dump_hex(const void *dat, size_t len_dat, uintptr_t base_off);
```

Hex + ASCII dump. Only emits when `SND_DEBUG=1`.

**Source:** `src/common/debug.c`

### `SND_DEBUG_MAX_LEN`

**Value:** `1024` — internal format buffer when using `OutputDebugStringA` path.

---

## File primitives (`sindri/primitives/files.h`)

### `snd_file_api_t` / `snd_file_win` / `snd_file_nt` / `snd_file_sys`

```c
typedef snd_status_t(WINAPI *snd_file_load_cb)(const char *path, snd_buffer_t *out_buffer);

typedef struct {
    snd_file_load_cb load;
} snd_file_api_t;

extern const snd_file_api_t snd_file_win;
extern const snd_file_api_t snd_file_nt;
extern const snd_file_api_t snd_file_sys;
```

Reads entire file into heap memory. Sets `free_routine = snd_buffer_free_heap`.

**Returns:** `SND_OK`, `SND_STATUS_INVALID_PATH`, `SND_STATUS_FILE_CREATE_FAILED`, `SND_STATUS_FILE_READ_FAILED`, etc.

**Sources:** `src/primitives/files/win.c`, `src/primitives/files/nt.c`, `src/primitives/files/sys.c`

---

## CRT manifest (`src/common/crt_manifest.c`)

Global linker symbols (not for direct use):

```c
void *memcpy(void *dest, const void *src, size_t count);
void *memset(void *dest, int c, size_t count);
```

Compiled when building CRT-less targets. Satisfies MSVC implicit codegen under `/NODEFAULTLIB`.

---

## Internal NT layouts (`include/sindri/internal/nt/`)

Not part of `sindri/common.h`, but replaces the former `internal/nt_defs.h` monolith.

### `base.h`

| Symbol | Description |
|---|---|
| `SND_NT_SUCCESS(s)` | `((NTSTATUS)(s)) >= 0` |
| `SND_OBJ_CASE_INSENSITIVE` | `0x00000040L` |
| `SND_InitializeObjectAttributes(p, n, a, r, s)` | Init object attributes macro |
| `SND_UNICODE_STRING` | NT counted wide string |
| `SND_OBJECT_ATTRIBUTES` | Object Manager attributes |
| `SND_CLIENT_ID` | Process/thread identifier |
| `snd_init_unicode_string(us, buf, char_count)` | Inline unicode string init |

### `api.h`

Function pointer typedefs: `SND_LdrLoadDll_t`, `SND_NtOpenSection_t`, `SND_NtMapViewOfSection_t`, `SND_NtClose_t`, `SND_NtAllocateVirtualMemory_t`, `SND_NtProtectVirtualMemory_t`, `SND_NtFreeVirtualMemory_t`, `SND_NtOpenProcess_t`, `SND_NtWriteVirtualMemory_t`, `SND_NtCreateThreadEx_t`.

### `peb.h`

Layouts: `SND_PEB`, `SND_PEB_LDR_DATA`, `SND_LDR_DATA_TABLE_ENTRY`, `SND_RTL_USER_PROCESS_PARAMETERS`, `SND_CURDIR`.

Include via `sindri/internal/nt.h`.

---

## Related documentation

- [Infrastructure concepts](common/infrastructure.md)
- [Status system architecture](architecture/status_system.md)
- [Hash manifest](config/hashes_manifest.md)

---

## Evasion


> [!NOTE]
> The evasion domain is planned for a future release of SindriKit. This page will be populated when the evasion context and its associated primitives are implemented.

See [implementation internals](injection/internals.md) for a high-level overview of planned capabilities.

---

## Injection


Public injection API under `include/sindri/injection/`. Include `sindri/injection.h` or `sindri.h` for the full surface.

---

## Shared Context (`sindri/injection/context.h`)

All injection techniques operate on `snd_inj_ctx_t`. Defined in `context.h`, not in technique-specific headers.

### `snd_inj_ctx_t`

| Field | Type | Description |
|---|---|---|
| `target_pid` | `DWORD` | PID of the target process (classic injection) |
| `target_process` | `HANDLE` | Handle opened/created via the process API |
| `remote_base` | `PVOID` | Remote allocation base |
| `remote_entry_point` | `PVOID` | Optional. If set, thread starts here instead of `remote_base` (PE path) |
| `remote_size` | `SIZE_T` | Size of the remote allocation |
| `remote_thread` | `HANDLE` | Handle from `proc_api->create_remote_thread` or `thread_api->queue_apc` |
| `remote_arg` | `PVOID` | Remote address of argument buffer (COFF execution) |
| `stage` | `snd_inj_stage_t` | Current pipeline stage |
| `payload` | `const snd_buffer_t *` | Source buffer (shellcode or locally baked image) |
| `proc_api` | `const snd_process_api_t *` | Injected remote process API |
| `thread_api` | `const snd_thread_api_t *` | Injected thread API (required for APC injection) |
| `target_image_path` | `const wchar_t *` | Path to the target process image (required for APC `create_process`) |

**Required before any classic chain:** `target_pid`, `proc_api`, and (for alloc/write) a valid `payload`.
**Required before any APC chain:** `target_image_path`, `proc_api`, `thread_api`, and a valid `payload`.

---

### `snd_inj_stage_t`

| Constant | Value | Description |
|---|---|---|
| `SND_INJ_STAGE_UNINITIALIZED` | 0 | Not started |
| `SND_INJ_STAGE_TARGET_ACQUIRED` | 1 | Process handle obtained |
| `SND_INJ_STAGE_MEMORY_ALLOCATED` | 2 | Remote memory allocated (RW) |
| `SND_INJ_STAGE_PAYLOAD_WRITTEN` | 3 | Payload written remotely |
| `SND_INJ_STAGE_PROTECTIONS_SET` | 4 | Remote memory set to RX |
| `SND_INJ_STAGE_EXECUTED` | 5 | Remote thread created |

---

### `snd_inj_stage_to_string`

Returns a human-readable stage name for logging and error context.

```c
const char *snd_inj_stage_to_string(snd_inj_stage_t stage);
```

**Source:** `src/injection/context.c`

---

### `snd_inj_cleanup`

Closes `remote_thread` and `target_process` through `proc_api->close_handle`, clears remote fields, resets stage to `UNINITIALIZED`.

```c
void snd_inj_cleanup(snd_inj_ctx_t *ctx);
```

Does not free loader-local mappings. No-op if `ctx` or `proc_api` is NULL.

---

## Classic Engine (`sindri/injection/classic/engine.h`)

Per-stage functions with strict stage validation. Each advances `ctx->stage` on success.

| Function | Required stage | `proc_api` callback | Advances to |
|---|---|---|---|
| `snd_inj_classic_open_target` | `UNINITIALIZED` | `open_process` | `TARGET_ACQUIRED` |
| `snd_inj_classic_alloc_remote` | `TARGET_ACQUIRED` | `alloc_remote` | `MEMORY_ALLOCATED` |
| `snd_inj_classic_write_payload` | `MEMORY_ALLOCATED` | `write_remote` | `PAYLOAD_WRITTEN` |
| `snd_inj_classic_set_protections` | `PAYLOAD_WRITTEN` | `protect_remote` | `PROTECTIONS_SET` |
| `snd_inj_classic_execute` | `PROTECTIONS_SET` | `create_remote_thread` | `EXECUTED` |

### `snd_inj_classic_open_target`

Opens the target with `PROCESS_ALL_ACCESS` (`0x001FFFFF`). Requires non-zero `target_pid`.

**Returns:** `SND_OK`, `SND_STATUS_PROCESS_OPEN_FAILED`, `SND_STATUS_INVALID_STAGE`, `SND_STATUS_NULL_POINTER`

---

### `snd_inj_classic_alloc_remote`

Allocates `payload->size` bytes remotely with `MEM_COMMIT | MEM_RESERVE` and `PAGE_READWRITE`. Sets `remote_size = payload->size`.

**Returns:** `SND_OK`, allocation errors from `proc_api`, `SND_STATUS_INVALID_STAGE`

---

### `snd_inj_classic_write_payload`

Writes `payload->data` (`remote_size` bytes) to `remote_base`.

**Returns:** `SND_OK`, `SND_STATUS_PROCESS_REMOTE_WRITE_FAILED`, `SND_STATUS_INVALID_STAGE`

---

### `snd_inj_classic_set_protections`

Transitions the full remote region to `PAGE_EXECUTE_READ`.

**Returns:** `SND_OK`, `SND_STATUS_PROCESS_REMOTE_PROTECT_FAILED`, `SND_STATUS_INVALID_STAGE`

---

### `snd_inj_classic_execute`

Creates a remote thread at `remote_entry_point` (if set) or `remote_base` as the fallback, with `NULL` as the parameter. Stores the handle in `remote_thread`.

**Returns:** `SND_OK`, `SND_STATUS_THREAD_REMOTE_CREATE_FAILED`, `SND_STATUS_INVALID_STAGE`

**Source:** `src/injection/classic/engine.c`

---

## Classic Chain (`sindri/injection/classic/chain.h`)

### `snd_inj_classic_shell`

Runs the full shellcode pipeline: open → alloc → write → protect → execute.

```c
snd_status_t snd_inj_classic_shell(snd_inj_ctx_t *ctx);
```

| Parameter | Description |
|---|---|
| `ctx` | Context with `target_pid`, `payload`, and `proc_api` set |

**Returns:** `SND_OK` or the failing stage's status

**Source:** `src/injection/classic/chain.c`

---

### `snd_inj_classic_pe`

Orchestrates local PE preparation via `snd_ldr_pe_ctx_t` and remote injection via `snd_inj_ctx_t`. See [implementation internals](injection/internals.md) for the interleaved step order.

```c
snd_status_t snd_inj_classic_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx);
```

| Parameter | Description |
|---|---|
| `ldr_ctx` | Loader context with `raw_source`, `mem_api`, and `mod_api` set |
| `inj_ctx` | Injection context with `target_pid` and `proc_api` set |

On success, `inj_ctx` reaches `SND_INJ_STAGE_EXECUTED` with `remote_entry_point` pointing at the remote entry point and `remote_base` pointing at the allocation base.

**Returns:** `SND_OK` or the failing loader/injection stage status

**Source:** `src/injection/classic/chain.c`

---

### `snd_inj_classic_coff`

Orchestrates local COFF preparation via `snd_ldr_coff_ctx_t` and remote injection via `snd_inj_ctx_t`.

```c
snd_status_t snd_inj_classic_coff(snd_ldr_coff_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx, const char *entry_point, void *args, int arg_len);
```

| Parameter | Description |
|---|---|
| `ldr_ctx` | Loader context with `raw_source`, `mem_api`, and `mod_api` set |
| `inj_ctx` | Injection context with `target_pid` and `proc_api` set |
| `entry_point` | The COFF symbol name to execute (e.g., `"go"`) |
| `args` | Buffer of BOF arguments to pass to the entry point |
| `arg_len` | Length of the arguments buffer |

The orchestrator allocates a remote buffer large enough for both the COFF sections and the arguments. It passes the remote arguments address to the entry point.

**Returns:** `SND_OK` or failing stage status

**Source:** `src/injection/classic/chain.c`

---

---

## APC Engine (`sindri/injection/apc/engine.h`)

Per-stage functions with strict stage validation for the APC technique.

| Function | Required stage | `proc_api` / `thread_api` callback | Advances to |
|---|---|---|---|
| `snd_inj_apc_create_target` | `UNINITIALIZED` | `create_process` | `TARGET_ACQUIRED` |
| `snd_inj_apc_alloc_remote` | `TARGET_ACQUIRED` | `alloc_remote` | `MEMORY_ALLOCATED` |
| `snd_inj_apc_write_payload` | `MEMORY_ALLOCATED` | `write_remote` | `PAYLOAD_WRITTEN` |
| `snd_inj_apc_set_protections` | `PAYLOAD_WRITTEN` | `protect_remote` | `PROTECTIONS_SET` |
| `snd_inj_apc_execute` | `PROTECTIONS_SET` | `queue_apc`, `resume_thread` | `EXECUTED` |

---

## APC Chain (`sindri/injection/apc/chain.h`)

### `snd_inj_apc_shell`

Runs the full shellcode pipeline for APC injection: open → alloc → write → protect → queue apc → execute.

```c
snd_status_t snd_inj_apc_shell(snd_inj_ctx_t *ctx);
```

| Parameter | Description |
|---|---|
| `ctx` | Context with `target_image_path`, `payload`, `proc_api` and `thread_api` set |

**Returns:** `SND_OK` or the failing stage's status

**Source:** `src/injection/apc/chain.c`

---

### `snd_inj_apc_pe`

Orchestrates local PE preparation via `snd_ldr_pe_ctx_t` and remote injection via `snd_inj_ctx_t`.

```c
snd_status_t snd_inj_apc_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx);
```

| Parameter | Description |
|---|---|
| `ldr_ctx` | Loader context with `raw_source`, `mem_api`, and `mod_api` set |
| `inj_ctx` | Injection context with `target_image_path`, `proc_api`, and `thread_api` set |

On success, `inj_ctx` reaches `SND_INJ_STAGE_EXECUTED`.

**Returns:** `SND_OK` or the failing loader/injection stage status

**Source:** `src/injection/apc/chain.c`

---

### `snd_inj_apc_coff`

Orchestrates local COFF preparation via `snd_ldr_coff_ctx_t` and remote injection via `snd_inj_ctx_t`.

```c
snd_status_t snd_inj_apc_coff(snd_ldr_coff_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx, const char *entry_point, void *args, int arg_len);
```

| Parameter | Description |
|---|---|
| `ldr_ctx` | Loader context with `raw_source`, `mem_api`, and `mod_api` set |
| `inj_ctx` | Injection context with `target_image_path`, `proc_api`, and `thread_api` set |
| `entry_point` | The COFF symbol name to execute (e.g., `"go"`) |
| `args` | Buffer of BOF arguments to pass to the entry point |
| `arg_len` | Length of the arguments buffer |

The orchestrator allocates a remote buffer large enough for both the COFF sections and the arguments. It passes the remote arguments address to the entry point.

**Returns:** `SND_OK` or failing stage status

**Source:** `src/injection/apc/chain.c`

## Related Documentation

- [Loader API](api_reference.md) — `snd_ldr_pe_ctx_t` and `snd_ldr_coff_ctx_t`
- [Process primitives](api_reference.md) — `snd_process_api_t` backends
- [State machines](architecture/state_machines.md) — global stage pattern

---

## Loaders


Public loader API for the **Reflective PE** technique under `include/sindri/loaders/pe/`. Include `sindri/loaders.h` or `sindri/loaders/pe.h`.

KnownDlls mapping is documented under [mapping/object-manager](api_reference.md) — not part of this API surface.

---

## Reflective Chain (`sindri/loaders/pe/chain.h`)

Primary entry points for end-to-end pe loading.

### `snd_ldr_pe_prepare_image`

Runs the full local preparation pipeline:

1. `snd_pe_parse(ctx->raw_source, FALSE, &ctx->pe)` — parses the raw PE
2. Architecture compatibility check (inline — `SND_IS_ARCH_COMPATIBLE`)
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

**Returns:** `SND_OK`, `SND_STATUS_DLL_INITIALIZATION_FAILED`, `SND_STATUS_IMAGE_ENTRY_POINT_MISSING`, `SND_STATUS_LOCAL_EXECUTION_BLOCKED` (remote-prepared image)

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
| `snd_ldr_pe_allocate_and_copy_image` | `PARSED` | Alloc + section copy + re-parse mapped; resolves entry point internally |
| `snd_ldr_pe_apply_relocations` | `SECTIONS_MAPPED` | Base relocation fixups |
| `snd_ldr_pe_resolve_imports` | `RELOCATED` | IAT patching via `mod_api` |
| `snd_ldr_pe_apply_memory_protections` | `IMPORTS_RESOLVED` | Per-page section protections |
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
5. `snd_ldr_coff_apply_memory_protections`

The caller must pre-initialize `ctx->raw_source`, `ctx->mem_api`, and `ctx->mod_api` before calling. Leaves context at `SND_COFF_STAGE_READY_FOR_EXECUTION`.

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
snd_status_t snd_ldr_coff_execute_image(snd_ldr_coff_ctx_t *ctx, const char *entry_name, char *bof_args, int bof_arg_len);
```

**Returns:** `SND_OK`, `SND_STATUS_COFF_LOADER_SYMBOL_MISSING`
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
| `snd_ldr_coff_apply_memory_protections` | `RELOCATED` | Per-section page protections based on characteristics |
| `snd_ldr_coff_free_mapped_image` | (any with `local_base`) | Frees mapping, resets toward `PARSED` |

#### `snd_ldr_coff_stage_to_string`

```c
const char *snd_ldr_coff_stage_to_string(snd_ldr_coff_stage_t stage);
```

Returns a human-readable stage name. Returns empty string when `SND_DEBUG=0`.

**Source:** `src/loaders/coff/engine.c`

### Stage validation errors

Engine functions return `SND_STATUS_INVALID_STAGE` with context naming expected vs actual stage. Allocation failure during section copy rolls back to `SND_STAGE_PARSED` and frees `local_base`.

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

- [Injection API](api_reference.md) — `snd_inj_classic_pe` reuses loader engine steps
- [PE parser API](api_reference.md) — underlying parse/reloc/import primitives
- [Memory / module primitives](api_reference.md) — `mem_api` / `mod_api` backends

---

## Parsers: Coff


Public COFF parser API under `include/sindri/parsers/coff/`. Include `sindri/parsers/coff.h` or `sindri/parsers.h` for the full surface.

---

## Core Parser (`sindri/parsers/coff/parser.h`)

### `snd_coff_parse`

Parses a raw COFF object file buffer into a `snd_coff_parser_t` context.

| Parameter | Type | Description |
| --- | --- | --- |
| `buf` | `const snd_buffer_t*` | Raw buffer containing the COFF file data |
| `parser` | `snd_coff_parser_t*` | Output context to populate |

**Returns:** `snd_status_t` — `SND_OK` or a contextual error status.

---

### `snd_coff_parser_t`

Context structure for the COFF parser. Pointers point directly into the memory space backed by the `source` buffer; they do not own memory.

| Field | Type | Description |
| --- | --- | --- |
| `source` | `snd_buffer_t` | Backing buffer (base pointer + tracked size) |
| `file_header` | `PIMAGE_FILE_HEADER` | Pointer to the COFF file header |
| `section_head` | `PIMAGE_SECTION_HEADER` | First section header entry |
| `symbol_table` | `PIMAGE_SYMBOL` | Pointer to the symbol table |
| `symbol_count` | `DWORD` | Number of symbols in the symbol table |
| `string_table` | `BYTE*` | Pointer to the string table |
| `string_table_size` | `DWORD` | Size of the string table in bytes |
| `is_64bit` | `BOOL` | `TRUE` for 64-bit architectures (e.g. AMD64) |
| `sections_count` | `DWORD` | Number of sections |

---

## Utilities (`sindri/parsers/coff/utils.h`)

### `snd_coff_get_symbol_name`

Retrieves the actual name of a COFF symbol, automatically abstracting inline names (<= 8 chars) and long names stored in the String Table.

| Parameter | Type | Description |
| --- | --- | --- |
| `parser` | `const snd_coff_parser_t*` | Parsed COFF context |
| `symbol` | `PIMAGE_SYMBOL` | Symbol structure to evaluate |
| `name_out` | `char*` | Output buffer to store the null-terminated name |
| `name_len` | `SIZE_T` | Size of the output buffer |

**Returns:** `snd_status_t` — `SND_OK` on success.

---

### `snd_coff_get_section_name`

Retrieves the actual name of a COFF section, handling long names that reference the string table.

| Parameter | Type | Description |
| --- | --- | --- |
| `parser` | `const snd_coff_parser_t*` | Parsed COFF context |
| `section` | `PIMAGE_SECTION_HEADER` | Section header |
| `name_out` | `char*` | Output buffer to store the null-terminated name |
| `name_len` | `SIZE_T` | Size of the output buffer |

**Returns:** `snd_status_t` — `SND_OK` on success.

---

### `snd_coff_raw_to_ptr`

Abstracted address translation. Converts a raw file offset to a pointer with bounds checking.

| Parameter | Type | Description |
| --- | --- | --- |
| `parser` | `const snd_coff_parser_t*` | Parsed COFF context |
| `raw_offset` | `DWORD` | Raw file offset (e.g. `PointerToRawData`) |
| `size` | `SIZE_T` | Size of the expected data for bounds checking |

**Returns:** `PVOID` — Direct pointer to the data, or `NULL` if out of bounds.

---

### `snd_coff_write_jmp_trampoline`

Writes an x64 absolute jump trampoline (`jmp [rip+0]`) safely to a target destination, avoiding strict aliasing or alignment violations.

| Parameter | Type | Description |
| --- | --- | --- |
| `dest` | `void*` | Destination buffer where the trampoline will be written (min 14 bytes) |
| `target_addr` | `void*` | Absolute target address to jump to |

**Returns:** `void`

---

## Symbols (`sindri/parsers/coff/symbols.h`)

### Constants and Types

| Symbol | Value | Description |
|---|---|---|
| `SND_COFF_MAX_SYMBOL_LEN` | `4096` | Maximum buffer size for symbol name extraction |

#### `snd_coff_sym_type_t`

Resolved runtime category of a COFF symbol:

| Constant | Description |
|---|---|
| `SND_COFF_SYM_TYPE_LOCAL` | Locally defined symbol |
| `SND_COFF_SYM_TYPE_BSS` | Uninitialized BSS allocation |
| `SND_COFF_SYM_TYPE_IMPORT` | External `__imp_MODULE$Func` import |
| `SND_COFF_SYM_TYPE_OTHER` | Unclassified symbol |

#### `snd_coff_import_info_t`

```c
typedef struct {
    char dll_name[128];   // DLL name parsed from __imp_DLLNAME$FunctionName
    char func_name[128];  // Function name parsed from __imp_DLLNAME$FunctionName
    BOOL is_imp;          // TRUE if the symbol had the __imp_ prefix
} snd_coff_import_info_t;
```

#### `snd_coff_decoded_sym_t`

Fully decoded COFF symbol, populated by `snd_coff_decode_symbol`:

```c
typedef struct {
    snd_coff_sym_type_t    type;    // Symbol category
    snd_coff_import_info_t import;  // Valid when type == SND_COFF_SYM_TYPE_IMPORT
    SIZE_T                 bss_size; // Valid when type == SND_COFF_SYM_TYPE_BSS
} snd_coff_decoded_sym_t;
```

---

### `snd_coff_get_symbol_by_index`

Retrieves a symbol by its zero-based index in the symbol table. Note: Symbols can have auxiliary records which take up index slots.

```c
PSND_IMAGE_SYMBOL snd_coff_get_symbol_by_index(const snd_coff_parser_t *parser, DWORD index);
```

**Returns:** `PSND_IMAGE_SYMBOL` — pointer to the symbol, or `NULL` if invalid/out of bounds.

---

### `snd_coff_find_symbol_by_name`

Finds a symbol by its exact string name.

```c
snd_status_t snd_coff_find_symbol_by_name(const snd_coff_parser_t *parser, const char *name, SIZE_T name_len,
                                          PSND_IMAGE_SYMBOL *symbol_out, DWORD *index_out);
```

| Parameter | Type | Description |
|---|---|---|
| `parser` | `const snd_coff_parser_t*` | Parsed COFF context |
| `name` | `const char*` | Name to search for (e.g. `"go"`, `"_MyFunction"`) |
| `name_len` | `SIZE_T` | Length of `name` (excluding null terminator) |
| `symbol_out` | `PSND_IMAGE_SYMBOL*` | Output pointer to store the located symbol |
| `index_out` | `DWORD*` | Optional output pointer to store the symbol's index |

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_SYMBOL_ENTRY_MISSING`

---

### `snd_coff_decode_symbol`

Decodes and classifies a COFF symbol into its runtime category, resolving sizes and import DLL/function names.

```c
snd_status_t snd_coff_decode_symbol(const snd_coff_parser_t *parser, PSND_IMAGE_SYMBOL sym,
                                    snd_coff_decoded_sym_t *decoded);
```

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_SYMBOL_NAKED_REJECTED`, `SND_STATUS_SYMBOL_TABLE_OVERFLOW`, `SND_STATUS_SYMBOL_DECODE_OVERFLOW`

---

## Relocations (`sindri/parsers/coff/relocations.h`)

### `snd_coff_get_relocations`

Retrieves the array of relocations for a specific section.

```c
snd_status_t snd_coff_get_relocations(const snd_coff_parser_t *parser, const SND_IMAGE_SECTION_HEADER *section,
                                      PSND_IMAGE_RELOCATION *relocations_out, DWORD *count_out);
```

**Returns:** `SND_OK` (even if section has zero relocations), `SND_STATUS_NULL_POINTER`, `SND_STATUS_RELOCATION_TABLE_MISSING`, `SND_STATUS_RELOCATION_TABLE_INVALID`, `SND_STATUS_RELOCATION_TABLE_OVERFLOW`, `SND_STATUS_RELOCATION_TABLE_TRUNCATED`

---

### `snd_coff_get_relocation_patch_size`

Returns the patch width in bytes for a given COFF relocation type. Used by the COFF loader to determine how many bytes to overwrite at each relocation site.

```c
snd_status_t snd_coff_get_relocation_patch_size(BOOL is_64bit, WORD reloc_type, SIZE_T *size_out);
```

| Parameter | Description |
|---|---|
| `is_64bit` | `TRUE` for AMD64 relocation types, `FALSE` for i386 types |
| `reloc_type` | Relocation type field from `SND_IMAGE_RELOCATION.Type` |
| `size_out` | Receives the patch width in bytes |

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_COFF_RELOCATION_TYPE_UNSUPPORTED`

---

## Parsers: Env


Public env parser API under `include/sindri/parsers/env/`. Include `sindri/parsers/env.h` or `sindri/parsers.h` for the full surface.

NT structure definitions consumed by these functions are in `include/sindri/internal/nt/peb.h`.

---

## PEB Module Resolution (`sindri/parsers/env/peb.h`)

### `snd_peb_get_local`

Force-inline accessor for the current process PEB.

**Returns:** `PSND_PEB` — pointer to the local Process Environment Block

Supported architectures: x64, x86, ARM64. Unsupported architectures fail at compile time.

---

### `snd_peb_get_module_base`

Locates a loaded module by walking `InMemoryOrderModuleList` and comparing `BaseDllName`.

```c
snd_status_t WINAPI snd_peb_get_module_base(const wchar_t *module_name, PVOID *out_base);
```

| Parameter | Description |
|---|---|
| `module_name` | Case-insensitive short name (e.g. `L"ntdll.dll"`) |
| `out_base` | Receives `DllBase` on success |

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_MODULE_NOT_FOUND`

**Source:** `src/parsers/env/peb.c`

---

### `snd_peb_get_module_base_hash`

Hash-based module lookup — no wide-string comparison at runtime.

```c
snd_status_t WINAPI snd_peb_get_module_base_hash(DWORD module_hash, PVOID *out_base);
```

| Parameter | Description |
|---|---|
| `module_hash` | Compile-time hash from `sindri_hashes.h` (e.g. `SND_HASH_NTDLL_DLL`) |
| `out_base` | Receives `DllBase` on success |

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_MODULE_NOT_FOUND`

---

## NTDLL State Management (`sindri/parsers/env/ntdll.h`)

Encapsulates global/active NTDLL state for syscall resolution.

### `snd_ntdll_entry_t`

Internal structure holding NTDLL parser state. Exposed for advanced consumers.

```c
typedef struct {
    PVOID           base;
    snd_pe_parser_t parser;
    BOOL            is_initialized;
} snd_ntdll_entry_t;
```

---

### `snd_ntdll_set_clean`

Sets a custom clean NTDLL image base address for SSN resolution and gadget scanning. Parses the provided image and stores the context.

```c
snd_status_t snd_ntdll_set_clean(PVOID clean_base);
```

| Parameter | Description |
|---|---|
| `clean_base` | Virtual memory address of the loaded/mapped clean NTDLL image |

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, or any error from `snd_pe_parse`

---

### `snd_ntdll_get_active_parser` / `snd_ntdll_get_active_base`

Lazily locates NTDLL from the process PEB on first call and initializes its parser context.

| Function | Output Parameter |
|---|---|
| `snd_ntdll_get_active_parser` | `const snd_pe_parser_t **out_parser` |
| `snd_ntdll_get_active_base` | `PVOID *out_base` |

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, or errors if NTDLL cannot be located from PEB.

---

### `snd_ntdll_get_clean_parser` / `snd_ntdll_get_clean_base`

Retrieves the parser or base address for the **clean** NTDLL set via `snd_ntdll_set_clean`. Both require that `snd_ntdll_set_clean` has been called beforehand.

```c
snd_status_t snd_ntdll_get_clean_parser(const snd_pe_parser_t **out_parser);
snd_status_t snd_ntdll_get_clean_base(PVOID *out_base);
```

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_NTDLL_CLEAN_NOT_INITIALIZED`

---

### `snd_ntdll_get_active_export` / `snd_ntdll_get_clean_export`

Resolves an export function address by DJB2 hash from the active PEB NTDLL or the clean NTDLL.

```c
snd_status_t snd_ntdll_get_active_export(DWORD func_hash, FARPROC *func_addr_out);
snd_status_t snd_ntdll_get_clean_export(DWORD func_hash, FARPROC *func_addr_out);
```

| Function | Usage |
|---|---|
| `snd_ntdll_get_active_export` | Locates native syscall procedures inside executable virtual memory |
| `snd_ntdll_get_clean_export` | Inspects unhooked NTDLL images for SSN extraction |

---

## NT Structures (`sindri/internal/nt/peb.h`)

These layouts are not public parser API but are referenced by env functions:

| Type | Description |
|---|---|
| `SND_PEB` | Process Environment Block |
| `SND_PEB_LDR_DATA` | Loader data with three module lists |
| `SND_LDR_DATA_TABLE_ENTRY` | Per-module entry (`DllBase`, `BaseDllName`, `FullDllName`, …) |
| `SND_RTL_USER_PROCESS_PARAMETERS` | Process parameters (command line, paths, environment) |
| `SND_UNICODE_STRING` | NT counted string |

---

## Usage as Export Forwarder Resolver

Both export functions accept `snd_module_resolver_cb`:

```c
typedef snd_status_t (WINAPI *snd_module_resolver_cb)(const wchar_t *module_name, PVOID *out_base);
```

`snd_peb_get_module_base` matches this signature and is the standard resolver for export forwarders:

```c
FARPROC func = NULL;
snd_status_t status = snd_pe_get_export_address_hash(
    &ntdll_parser,
    SND_HASH_NTOPENSECTION,
    &func,
    snd_peb_get_module_base
);
```

The hash-based export function uses the same wide-name resolver — not `snd_module_resolver_hash_cb`.

---

## Status Codes

| Code | Meaning |
|---|---|
| `SND_STATUS_MODULE_NOT_FOUND` | Module not present in the PEB loader list |
| `SND_STATUS_NTDLL_CLEAN_NOT_INITIALIZED` | `snd_ntdll_set_clean` was not called before accessing clean NTDLL state |
| `SND_STATUS_PEB_GET_FAILED` | Failed to locate the Process Environment Block |
| `SND_STATUS_PEB_LDR_NOT_INITIALIZED` | PEB loader data is not yet initialized |
| `SND_STATUS_MODULE_LIST_CORRUPTED` | PEB module list traversal detected a corrupted Flink/Blink chain |
| `SND_STATUS_PROCESS_PARAMS_NOT_FOUND` | Process parameters (`RTL_USER_PROCESS_PARAMETERS`) not accessible |
| `SND_STATUS_PEB_PROCESS_PARAMETERS_NOT_FOUND` | Reserved for future process-parameter helpers |
| `SND_STATUS_NULL_POINTER` | NULL output pointer |

---

## Related Documentation

- [PE export resolution](api_reference.md) — `snd_pe_get_export_address`, forwarders
- [Module primitives](api_reference.md) — `snd_mod_nt` wiring
- [Dependency Injection](architecture/dependency_injection.md) — resolver callback types in `os_api.h`

---

## Parsers: Pe


Public PE parser API under `include/sindri/parsers/pe/`. Include `sindri/parsers/pe.h` or `sindri/parsers.h` for the full surface.

---

## Core Parser (`sindri/parsers/pe/parser.h`)

### `snd_pe_parse`

Validates and parses a raw or mapped PE buffer into a `snd_pe_parser_t` context. Checks the DOS signature (`MZ`), NT signature (`PE\0\0`), and Optional Header magic (`0x10B` PE32, `0x20B` PE32+). When `is_mapped == TRUE` and `source->size == SND_SYS_DLL_SIZE_DEFAULT`, automatically updates `parser->source.size` to `OptionalHeader.SizeOfImage`.

| Parameter | Type | Description |
|---|---|---|
| `source` | `const snd_buffer_t*` | Buffer containing PE data (pointer + size) |
| `is_mapped` | `BOOL` | `FALSE` for raw file layout, `TRUE` for virtually mapped image |
| `parser` | `snd_pe_parser_t*` | Output context to populate |

**Returns:** `snd_status_t` — `SND_OK` or a parse error (`SND_STATUS_NULL_POINTER`, `SND_STATUS_HEADER_DOS_SIGNATURE_INVALID`, `SND_STATUS_HEADER_DOS_TRUNCATED`, `SND_STATUS_HEADER_OFFSET_INVALID`, `SND_STATUS_HEADER_NT_TRUNCATED`, `SND_STATUS_HEADER_NT_SIGNATURE_INVALID`, `SND_STATUS_HEADER_OPTIONAL_SIGNATURE_INVALID`).

**Source:** `src/parsers/pe/parser.c`

---

### `snd_pe_parser_t`

| Field | Type | Description |
|---|---|---|
| `source` | `snd_buffer_t` | Backing buffer (base pointer + tracked size) |
| `dos` | `PSND_IMAGE_DOS_HEADER` | DOS header pointer into `source` |
| `nt.nt32` | `PSND_IMAGE_NT_HEADERS32` | Valid when `is_64bit == FALSE` |
| `nt.nt64` | `PSND_IMAGE_NT_HEADERS64` | Valid when `is_64bit == TRUE` |
| `section_head` | `PSND_IMAGE_SECTION_HEADER` | First section header entry |
| `string_table` | `BYTE*` | COFF string table (raw files only; `NULL` for mapped images) |
| `is_64bit` | `BOOL` | `TRUE` for PE32+ |
| `is_dll` | `BOOL` | `TRUE` if `IMAGE_FILE_DLL` is set |
| `is_mapped` | `BOOL` | Layout mode passed to `snd_pe_parse` |
| `lfanew` | `SIZE_T` | Byte offset of the NT headers from the image base |
| `sections_count` | `DWORD` | Number of section headers (clamped to buffer bounds) |
| `imports_rva` | `DWORD` | Cached import directory RVA (or 0 if missing) |
| `import_size` | `DWORD` | Cached import directory size (or 0 if missing) |

---

### Macros

| Macro | Value | Description |
|---|---|---|
| `SND_PE_GET_NT_FIELD(parser, field)` | — | Reads an Optional Header field without bitness branching |
| `SND_SYS_DLL_SIZE_DEFAULT` | `0x1000` | Bootstrap bound for in-memory system DLL parsing (see parser.h for CAUTION notes) |

---

## Utilities (`sindri/parsers/pe/utils.h`)

| Macro | Value | Description |
|---|---|---|
| `SND_PE_MIN_FILE_ALIGNMENT` | `512` | Minimum valid file alignment (RVA translation) |

### `snd_pe_get_directory`

Safely retrieves a data directory entry by index.

```c
snd_status_t snd_pe_get_directory(const snd_pe_parser_t *parser, DWORD index, SND_IMAGE_DATA_DIRECTORY *dir_out);
```

| Parameter | Description |
|---|---|
| `parser` | Parsed PE context |
| `index` | e.g. `SND_IMAGE_DIRECTORY_ENTRY_IMPORT`, `SND_IMAGE_DIRECTORY_ENTRY_EXPORT` |
| `dir_out` | Output `SND_IMAGE_DATA_DIRECTORY` |

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_DIRECTORY_ENTRY_MISSING`, `SND_STATUS_DIRECTORY_ENTRY_INVALID`

---

### `snd_pe_rva_to_ptr`

Translates an RVA to a pointer within the parser's tracked buffer with bounds checking.

```c
PVOID snd_pe_rva_to_ptr(const snd_pe_parser_t *parser, DWORD rva, SIZE_T size);
```

| Parameter | Description |
|---|---|
| `parser` | Parsed PE context |
| `rva` | Relative Virtual Address |
| `size` | Expected data size at the target (for bounds check) |

**Returns:** `PVOID` — data pointer, or `NULL` if out of bounds. For mapped images: `source.base + rva`. For raw files: signed reverse section-table translation with file alignment and overflow protections.

---

### `snd_pe_get_entry_point`

Computes the absolute entry point from `AddressOfEntryPoint` via `snd_pe_rva_to_ptr`.

```c
PVOID snd_pe_get_entry_point(const snd_pe_parser_t *parser);
```

**Returns:** `void*` — entry point address, or `NULL` if absent/invalid.

---

### `snd_pe_get_tls_callbacks`

Retrieves the TLS callback array pointer from the TLS data directory (supports PE32 and PE32+).

```c
PVOID snd_pe_get_tls_callbacks(const snd_pe_parser_t *parser);
```

**Returns:** `PVOID` — callback array pointer, or `NULL` if TLS directory is absent/invalid.

---

### `snd_pe_get_page_protection_flags`

Calculates Win32 `PAGE_*` protection flags for a relative page offset, based on PE section characteristics.

```c
DWORD snd_pe_get_page_protection_flags(const snd_pe_parser_t *pe, SIZE_T page_offset);
```

**Returns:** `SND_PAGE_*` — protection flags for the page.

---

## Export Resolution (`sindri/parsers/pe/exports.h`)

Both export functions share a unified implementation and the **same forwarder resolver type** (`snd_module_resolver_cb`).

### `snd_pe_get_export_address`

Resolves an export by name or ordinal from a parsed PE image.

```c
snd_status_t snd_pe_get_export_address(const snd_pe_parser_t *parser, const char *func_name,
                                        FARPROC *func_addr_out, snd_module_resolver_cb resolver);
```

| Parameter | Type | Description |
|---|---|---|
| `parser` | `const snd_pe_parser_t*` | Initialized PE parser context |
| `func_name` | `const char*` | Export name, or ordinal via `SND_MAKEINTRESOURCE(n)` |
| `func_addr_out` | `FARPROC*` | Receives resolved address |
| `resolver` | `snd_module_resolver_cb` | Wide-name module lookup for forwarders; `NULL` fails forwarders |

**Returns:** `SND_OK`, `SND_STATUS_EXPORT_SYMBOL_MISSING`, `SND_STATUS_EXPORT_DIRECTORY_NULL`, `SND_STATUS_EXPORT_TABLE_EMPTY`, `SND_STATUS_EXPORT_DIRECTORY_INVALID`, `SND_STATUS_EXPORT_DIRECTORY_OVERFLOW`, `SND_STATUS_EXPORT_ORDINAL_INVALID`, `SND_STATUS_EXPORT_ORDINAL_OUT_OF_RANGE`, `SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED`, `SND_STATUS_EXPORT_FORWARDER_EXCEEDED`, `SND_STATUS_EXPORT_FORWARDER_INVALID`

---

### `snd_pe_get_export_address_hash`

Hash-based export resolution using compile-time hashes from `sindri_hashes.h`.

```c
snd_status_t snd_pe_get_export_address_hash(const snd_pe_parser_t *parser, DWORD func_hash,
                                             FARPROC *func_addr_out, snd_module_resolver_cb resolver);
```

| Parameter | Type | Description |
|---|---|---|
| `func_hash` | `DWORD` | Pre-computed hash of the export name |
| `resolver` | `snd_module_resolver_cb` | Same wide-name resolver used by the string variant |

All other parameters match `snd_pe_get_export_address`.

---

### `snd_pe_enumerate_exports`

Enumerates all named exported symbols from a parsed PE image via a callback.

```c
typedef BOOL (*snd_pe_export_enum_cb)(const char *func_name, WORD ordinal, PVOID func_addr, PVOID user_ctx);

snd_status_t snd_pe_enumerate_exports(const snd_pe_parser_t *parser, snd_pe_export_enum_cb callback, PVOID user_ctx);
```

| Parameter | Description |
|---|---|
| `parser` | Parsed PE context |
| `callback` | Invoked for each valid export; return `FALSE` to stop early |
| `user_ctx` | Arbitrary pointer passed through to callback |

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_EXPORT_DIRECTORY_NULL`, `SND_STATUS_EXPORT_DIRECTORY_INVALID`

---

### `SND_FWD_MAX_DEPTH`

**Value:** `4` — maximum recursion depth for export forwarder chains.

---

## Import Parsing (`sindri/parsers/pe/imports.h`)

The PE parser exposes three low-level import iteration primitives. The reflective loader's `snd_ldr_pe_resolve_imports` (in `src/loaders/pe/engine.c`) consumes these to walk the Import Descriptor table and patch the IAT.

### `snd_pe_import_thunk_t`

Represents a parsed, normalized import thunk entry, abstracting 32-bit vs 64-bit layout differences.

| Field | Type | Description |
|---|---|---|
| `name` | `const char*` | Target function name (NULL if imported by ordinal) |
| `ordinal` | `WORD` | Ordinal number (0 if imported by name) |
| `is_ordinal` | `BOOL` | `TRUE` if importing by ordinal |
| `iat_slot` | `PVOID` | Pointer to the IAT entry in memory to write to |

### `snd_pe_get_import_descriptor`

Retrieves a specific import descriptor by zero-based index.

```c
snd_status_t snd_pe_get_import_descriptor(const snd_pe_parser_t *parser, DWORD index,
                                           const SND_IMAGE_IMPORT_DESCRIPTOR **out_desc);
```

Sets `*out_desc = NULL` when the null-terminator descriptor is reached (end of import table).

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_IMPORT_DESCRIPTOR_OVERFLOW`, `SND_STATUS_IMPORT_DESCRIPTOR_INVALID`

---

### `snd_pe_get_import_name`

Resolves the DLL name string from an import descriptor.

```c
snd_status_t snd_pe_get_import_name(const snd_pe_parser_t *parser,
                                     const SND_IMAGE_IMPORT_DESCRIPTOR *desc,
                                     const char **out_name);
```

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_IMPORT_NAME_INVALID`

---

### `snd_pe_get_import_thunk`

Retrieves and parses a specific import thunk from an import descriptor by zero-based index.

```c
snd_status_t snd_pe_get_import_thunk(const snd_pe_parser_t *parser,
                                      const SND_IMAGE_IMPORT_DESCRIPTOR *desc,
                                      DWORD thunk_index, snd_pe_import_thunk_t *out_thunk);
```

`iat_slot` and `name` in `out_thunk` are `NULL` when the null-terminator thunk is reached (end of thunk array).

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_IMPORT_THUNK_OVERFLOW`, `SND_STATUS_IMPORT_THUNK_OUT_OF_BOUNDS`, `SND_STATUS_IMPORT_THUNK_INVALID`, `SND_STATUS_IMPORT_NAME_INVALID`

---

### Ordinal Macros

| Macro | Description |
|---|---|
| `SND_PE_SNAP_BY_ORDINAL32(ordinal)` | High bit set on 32-bit thunk |
| `SND_PE_SNAP_BY_ORDINAL64(ordinal)` | High bit set on 64-bit thunk |
| `SND_PE_ORDINAL(ordinal)` | Extracts 16-bit ordinal value |

---

## Base Relocations (`sindri/parsers/pe/relocations.h`)

The PE parser exposes two low-level relocation iteration primitives. The reflective loader's `snd_ldr_pe_apply_relocations` (in `src/loaders/pe/engine.c`) consumes these to apply fixups.

### `snd_pe_reloc_entry_t`

```c
typedef struct {
    WORD  type;      // Relocation type (e.g., SND_IMAGE_REL_BASED_HIGHLOW, SND_IMAGE_REL_BASED_DIR64)
    DWORD patch_rva; // RVA where the fixup needs to be applied
    PVOID patch_ptr; // Host-memory pointer to the location to patch (NULL for ABSOLUTE/padding entries)
} snd_pe_reloc_entry_t;
```

### `snd_pe_get_reloc_block`

Iterates relocation blocks sequentially using a caller-managed cursor.

```c
snd_status_t snd_pe_get_reloc_block(const snd_pe_parser_t *parser, SIZE_T *cursor,
                                     const SND_IMAGE_BASE_RELOCATION **out_block,
                                     DWORD *out_entries_count);
```

| Parameter | Description |
|---|---|
| `cursor` | Set to `0` on first call; updated to point to the next block on each success |
| `out_block` | Set to `NULL` when the end of the relocation directory is reached |
| `out_entries_count` | Number of entries in the current block |

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_RELOCATION_DIRECTORY_TRUNCATED`, `SND_STATUS_RELOCATION_DIRECTORY_OVERFLOW`, `SND_STATUS_RELOCATION_DIRECTORY_OUT_OF_BOUNDS`, `SND_STATUS_RELOCATION_BLOCK_INVALID`

---

### `snd_pe_get_reloc_entry`

Parses a specific relocation entry within a block by zero-based index.

```c
snd_status_t snd_pe_get_reloc_entry(const snd_pe_parser_t *parser,
                                     const SND_IMAGE_BASE_RELOCATION *block,
                                     DWORD entry_index, snd_pe_reloc_entry_t *out_entry);
```

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_RELOCATION_ENTRY_OVERFLOW`, `SND_STATUS_PE_RELOCATION_TYPE_UNSUPPORTED`, `SND_STATUS_PE_RELOCATION_PATCH_OUT_OF_RANGE`

---

## Section Helpers (`sindri/parsers/pe/section.h`)

| Function | Notes |
|---|---|
| `snd_pe_section_name` | Resolves section names including COFF string table long-name encoding |
| `snd_pe_section_copy_size` | Raw-to-virtual copy size (min of `SizeOfRawData`, `VirtualSize`) |
| `snd_pe_section_loaded_size` | Final virtual section size (`VirtualSize` or aligned `SizeOfRawData`) |

---

## Primitives: Execution


Public headers: `include/sindri/primitives/ffi.h`, `include/sindri/primitives/heavens_gate.h`.

For the syscall surface (`snd_syscall_resolve`, `snd_syscall_direct_invoke_asm`, `snd_syscall_indirect_invoke_asm`, strategy pipeline), see [syscalls/api_reference.md](api_reference.md).

---

## Dynamic Invocation (`sindri/primitives/ffi.h`)

### `snd_ffi_execute`

Invokes an arbitrary resolved function pointer using a generic `UINT_PTR` argument array. Register placement, stack alignment, and shadow space are handled in MASM (`snd_ffi_bridge_x64` / `snd_ffi_bridge_x86`).

```c
UINT_PTR snd_ffi_execute(PVOID pFunctionAddress, DWORD dwArgCount, const UINT_PTR *pArgs);
```

| Parameter | Type | Description |
|---|---|---|
| `pFunctionAddress` | `PVOID` | Target function; returns `0` immediately if NULL |
| `dwArgCount` | `DWORD` | Number of arguments; may be `0` |
| `pArgs` | `const UINT_PTR *` | Argument array; may be NULL when count is `0` |

**Returns:** `UINT_PTR` — value returned by the target, or `0` on error.

**Error / early-exit conditions:**

| Condition | Result |
|---|---|
| `pFunctionAddress == NULL` | Returns `0` |
| `dwArgCount > 0 && pArgs == NULL` | Returns `0` |
| Unsupported architecture | Returns `0` (compile-time `#error` on non-Windows) |

**Callee-saved registers** (`RBX`, `RBP`, `RDI`, `RSI`, `R12`–`R15` on x64) are preserved by the assembly bridge. The caller must supply arguments compatible with the target's actual signature and calling convention.

**Source:** `src/primitives/execution/ffi/ffi_invoke.c`

---

## Heaven's Gate (`sindri/primitives/heavens_gate.h`)

### `snd_is_wow64`

Detects WoW64 by reading `WOW32Reserved` from the TEB (`__readfsdword(0xC0)` on x86). No Win32 API calls.

```c
BOOL snd_is_wow64(void);
```

| Build | Behavior |
|---|---|
| `_WIN32` (x86) | `TRUE` when `WOW32Reserved != NULL` |
| `_WIN64` | Always `FALSE` |

---

### `snd_hg_execute_64`

Transitions to 64-bit mode via CS selector `0x33` and invokes a 64-bit target. Arguments are copied into a fixed six-slot buffer before entering the ASM bridge (`snd_hg_invoke_x86`).

```c
snd_status_t snd_hg_execute_64(
    UINT64 pFunctionAddress,
    DWORD dwArgCount,
    const UINT64 *pArgs,
    UINT64 *pResult);
```

| Parameter | Type | Description |
|---|---|---|
| `pFunctionAddress` | `UINT64` | 64-bit virtual address of target |
| `dwArgCount` | `DWORD` | Number of 64-bit arguments (max **6**, Microsoft x64 ABI) |
| `pArgs` | `const UINT64 *` | Argument array; required when count > 0 |
| `pResult` | `UINT64 *` | Receives `RAX`; may be NULL (result discarded) |

**Returns:**

| Status | Cause |
|---|---|
| `SND_OK` | Invocation completed |
| `SND_STATUS_NULL_POINTER` | `pFunctionAddress` is zero |
| `SND_STATUS_TOO_MANY_ARGUMENTS` | `dwArgCount` > 6 |
| `SND_STATUS_INVALID_PARAMETERS_COMBINATION` | `dwArgCount` > 0 but `pArgs` is NULL |
| `SND_STATUS_ARCH_MISMATCH` | Native x64 build, non-WoW64 host, or pure 32-bit OS |

**Source:** `src/primitives/execution/heavens_gate/heavens_gate.c`

---

## Internal ASM symbols (not public API)

| Symbol | File | Role |
|---|---|---|
| `snd_ffi_bridge_x64` | `ffi/asm/ffi_x64.asm` | x64 fast-call + shadow space |
| `snd_ffi_bridge_x86` | `ffi/asm/ffi_x86.asm` | Reverse-order stack push, ESP restore |
| `snd_hg_invoke_x86` | `heavens_gate/asm/heavens_gate_x86.asm` | Far return to CS `0x33`, x64 call |
| `snd_syscall_direct_invoke_asm` | `syscalls/invokers/direct_x64.asm` or `direct_x86.asm` | Direct `syscall` / `sysenter` instruction stub |
| `snd_syscall_indirect_invoke_asm` | `syscalls/invokers/indirect_x64.asm` or `indirect_x86.asm` | Indirect syscall via NTDLL gadget |
| `snd_syscall_spoofed_invoke_asm` | `syscalls/invokers/spoofed_x64.asm` or `spoofed_x86.asm` | Spoofed syscall via Fat Frame gadget |

These are linked into `sindri::engine`; callers use the C wrappers above.

---

## Primitives: Mapping


This page documents the section mapping capabilities exported by the framework. All type definitions live in `include/sindri/primitives/os_api.h`; pre-built instances are declared in `include/sindri/primitives/mapping.h`.

---

## Mapping API Instances (`sindri/primitives/mapping.h`)

Three pre-built instances of the `snd_mapping_api_t` interface are exported globally.

| Symbol | Backend | Source |
|---|---|---|
| `snd_map_win` | Win32 API (`OpenFileMappingW`, `MapViewOfFile`, `CloseHandle`) | `src/primitives/mapping/win.c` |
| `snd_map_nt` | NT API via PEB + EAT resolution (`NtOpenSection`, `NtMapViewOfSection`, `NtClose`) | `src/primitives/mapping/nt.c` |
| `snd_map_sys` | Direct syscalls via SSN resolution + ASM stub | `src/primitives/mapping/sys.c` |

---

## The Interface (`sindri/primitives/os_api.h`)

### `snd_mapping_api_t`

Function pointer table defining the section mapping contract.

| Field | Signature | Description |
|---|---|---|
| `open` | `snd_status_t (*)(const wchar_t *section_name, HANDLE *out_handle)` | Opens a named section object with read access |
| `view` | `snd_status_t (*)(HANDLE section_handle, PVOID *out_base, SIZE_T *out_size)` | Maps a read-only view into the current process |
| `close` | `snd_status_t (*)(HANDLE handle)` | Closes the section handle |

#### `open`

| Parameter | Description |
|---|---|
| `section_name` | Section object name. NT backends accept absolute Object Manager paths (e.g. `\KnownDlls\ntdll.dll`). The Win32 backend accepts DOS or Win32 namespace paths only. |
| `out_handle` | Receives the opened section handle on success |

**Status codes:** `SND_STATUS_MAPPING_OPEN_FAILED` (with OS or NTSTATUS detail), `SND_STATUS_NULL_POINTER`

#### `view`

| Parameter | Description |
|---|---|
| `section_handle` | Handle returned by `open` |
| `out_base` | Receives the base address of the mapped view |
| `out_size` | Receives the size of the mapped region |

All backends map with read-only protection (`PAGE_READONLY` / `FILE_MAP_READ`). The Win32 backend derives `out_size` from `VirtualQuery`; NT and syscall backends use the size returned by `NtMapViewOfSection`.

**Status codes:** `SND_STATUS_MAPPING_VIEW_FAILED`, `SND_STATUS_NULL_POINTER`

#### `close`

Closes the section handle. The mapped view remains valid after the handle is closed (standard Windows section semantics).

**Status codes:** `SND_STATUS_MAPPING_HANDLE_CLOSE_FAILED`, `SND_STATUS_NULL_POINTER`

---

## Object Manager Helper (`sindri/primitives/object_manager.h`)

### `snd_om_knowndll_map`

Maps a DLL from the `\KnownDlls` (x64) or `\KnownDlls32` (x86) Object Manager directory into the current process.

```c
snd_status_t snd_om_knowndll_map(
    const snd_mapping_api_t *config,
    const wchar_t *dll_name,
    PVOID *out_base_address
);
```

| Parameter | Description |
|---|---|
| `config` | Mapping backend to use (`&snd_map_nt` or `&snd_map_sys` for KnownDlls paths) |
| `dll_name` | DLL file name only (e.g. `L"ntdll.dll"`) — the helper prepends `SND_TARGET_KNOWNDLLS_DIR` |
| `out_base_address` | Receives the base address of the mapped view |

**Status codes:** `SND_STATUS_NULL_POINTER`, `SND_STATUS_MAPPING_OPEN_FAILED`, `SND_STATUS_MAPPING_VIEW_FAILED`

**Source:** `src/primitives/object_manager/knowndlls.c`

---

## Primitives: Memory


This page documents the local memory capabilities exported by the framework. Type definitions live in `include/sindri/primitives/os_api.h`; pre-built instances are declared in `include/sindri/primitives/memory.h`.

---

## Memory API Instances (`sindri/primitives/memory.h`)

Three pre-built instances of the `snd_memory_api_t` interface are exported globally. These pointers are typically assigned to `ctx.mem_api` during loader or injection initialization.

| Symbol | Backend | Source |
|---|---|---|
| `snd_mem_win` | Win32 API (`VirtualAlloc`, `VirtualProtect`, `VirtualFree`) | `src/primitives/memory/win.c` |
| `snd_mem_nt` | NT API via PEB + EAT resolution (`NtAllocateVirtualMemory`, etc.) | `src/primitives/memory/nt.c` |
| `snd_mem_sys` | Direct syscalls via SSN resolution + ASM stub | `src/primitives/memory/sys.c` |

---

## The Interface (`sindri/primitives/os_api.h`)

### `snd_memory_api_t`

Function pointer table defining the local memory contract. Operators can implement custom instances to route operations through alternative mechanisms (hypervisors, vulnerable drivers, etc.).

| Field | Signature | Description |
|---|---|---|
| `alloc` | `snd_status_t (*)(LPVOID address, SIZE_T size, DWORD alloc_type, DWORD protect, LPVOID *out_address)` | Allocates virtual memory in the current process |
| `free` | `snd_status_t (*)(LPVOID address, SIZE_T size, DWORD free_type)` | Frees or decommits virtual memory |
| `protect` | `snd_status_t (*)(LPVOID address, SIZE_T size, DWORD new_protect, DWORD *old_protect)` | Changes page protections |

#### `alloc`

| Parameter | Description |
|---|---|
| `address` | Preferred base address, or `NULL` to let the OS choose |
| `size` | Number of bytes to allocate |
| `alloc_type` | Win32 allocation flags (`MEM_COMMIT`, `MEM_RESERVE`, etc.) |
| `protect` | Initial page protection (`PAGE_READWRITE`, `PAGE_EXECUTE_READ`, etc.) |
| `out_address` | Receives the allocated base address on success |

**Status codes:** `SND_STATUS_ALLOC_FAILED`, `SND_STATUS_NULL_POINTER`

#### `free`

| Parameter | Description |
|---|---|
| `address` | Base address of the region to free |
| `size` | Size of the region (`0` when using `MEM_RELEASE`) |
| `free_type` | Win32 free flags (`MEM_RELEASE`, `MEM_DECOMMIT`) |

The NT and syscall backends treat a zero `size` on free as a no-op success.

**Status codes:** `SND_STATUS_FREE_FAILED`, `SND_STATUS_NULL_POINTER`

#### `protect`

| Parameter | Description |
|---|---|
| `address` | Base address of the region |
| `size` | Size of the region to reprotect |
| `new_protect` | Desired page protection |
| `old_protect` | Optional; receives the previous protection on success |

**Status codes:** `SND_STATUS_PROTECT_FAILED`, `SND_STATUS_NULL_POINTER`

---

## Dependency Injection

Memory backends are injected into domain contexts at initialization:

```c
snd_ldr_pe_ctx_t ctx = {0};
ctx.mem_api = &snd_mem_sys;  // or &snd_mem_nt, &snd_mem_win
```

The loader engine calls `ctx.mem_api->alloc`, `ctx.mem_api->protect`, and `ctx.mem_api->free` throughout the reflective loading pipeline. See [Dependency Injection](architecture/dependency_injection.md) for the full pattern.

---

## Primitives: Modules


This page documents the module and import resolution capabilities exported by the framework. Type definitions live in `include/sindri/primitives/os_api.h`; pre-built instances are declared in `include/sindri/primitives/modules.h`.

---

## Module API Instances (`sindri/primitives/modules.h`)

Two pre-built instances of the `snd_module_api_t` interface are exported globally. These pointers are typically assigned to `ctx.mod_api` during loader initialization.

| Symbol | Backend | Source |
|---|---|---|
| `snd_mod_win` | `LoadLibraryExA` / `GetProcAddress` / `GetModuleHandleW` | `src/primitives/modules/win.c` |
| `snd_mod_nt` | PEB walking + hash-based EAT resolution (`LdrLoadDll`, `snd_pe_get_export_address*`) | `src/primitives/modules/nt.c` |

> [!NOTE]
> Only `snd_mod_nt` populates the hash-based callbacks. On `snd_mod_win`, `get_proc_address_hash` and `get_module_base_hash` are `NULL`.

---

## The Interface (`sindri/primitives/os_api.h`)

### `snd_module_api_t`

Function pointer table defining the module and import resolution contract.

| Field | Signature | Description |
|---|---|---|
| `load_library` | `snd_status_t (*)(const char *module_name, HMODULE *out_module)` | Loads or resolves a module into the process |
| `get_proc_address` | `snd_status_t (*)(HMODULE hModule, const char *proc_name, FARPROC *out_proc)` | Resolves an exported symbol by name |
| `get_module_base` | `snd_status_t (*)(const wchar_t *module_name, PVOID *out_base)` | Retrieves the base address of an already-loaded module |
| `get_proc_address_hash` | `snd_status_t (*)(HMODULE hModule, DWORD proc_hash, FARPROC *out_proc)` | Hash-based export resolution |
| `get_module_base_hash` | `snd_status_t (*)(DWORD module_hash, PVOID *out_base)` | Hash-based module lookup via PEB walk |

#### `load_library`

| Parameter | Description |
|---|---|
| `module_name` | ASCII DLL name (e.g. `"kernel32.dll"`) |
| `out_module` | Receives the module handle / base address on success |

On `snd_mod_nt`, the backend first checks whether the module is already loaded via PEB walk before calling `LdrLoadDll`.

**Status codes:** `SND_STATUS_MODULE_LOAD_FAILED`, `SND_STATUS_NULL_POINTER`

#### `get_proc_address` / `get_proc_address_hash`

| Parameter | Description |
|---|---|
| `hModule` | Module base address |
| `proc_name` / `proc_hash` | Export name or compile-time hash (`sindri_hashes.h`) |
| `out_proc` | Receives the resolved function pointer |

**Status codes:** `SND_STATUS_PROC_RESOLVE_FAILED`, `SND_STATUS_NULL_POINTER`

#### `get_module_base` / `get_module_base_hash`

| Parameter | Description |
|---|---|
| `module_name` / `module_hash` | Wide module name or compile-time hash |
| `out_base` | Receives the module base address |

Does not load the module — returns a not-found status if the module is not in the PEB list.

**Status codes (`snd_mod_nt`):** `SND_STATUS_MODULE_NOT_FOUND`, `SND_STATUS_NULL_POINTER`

**Status codes (`snd_mod_win` `get_module_base`):** `SND_STATUS_MODULE_BASE_GET_FAILED`, `SND_STATUS_NULL_POINTER`

---

## Related Parsers (`sindri/parsers/env/peb.h`)

PEB walking primitives used by `snd_mod_nt` are documented under the parsers domain:

- `snd_peb_get_module_base` — wide-name PEB walk
- `snd_peb_get_module_base_hash` — hash-based PEB walk
- `snd_peb_get_local` — inline accessor for the current process PEB

Export parsing (`snd_pe_get_export_address`, `snd_pe_get_export_address_hash`) lives in `sindri/parsers/pe/exports.h`. Both functions share the same forwarder resolver type (`snd_module_resolver_cb`); `snd_peb_get_module_base` is the typical wiring.

---

## Dependency Injection

Module backends are injected into loader contexts at initialization:

```c
snd_ldr_pe_ctx_t ctx = {0};
ctx.mod_api = &snd_mod_nt;  // or &snd_mod_win
```

The reflective loader and `snd_pe_resolve_imports` call `ctx.mod_api` throughout the import fixup pipeline. See [Dependency Injection](architecture/dependency_injection.md).

---

## Primitives: Process


This page documents the remote process capabilities exported by the framework. Type definitions live in `include/sindri/primitives/os_api.h`; pre-built instances are declared in `include/sindri/primitives/process.h`.

---

## Process API Instances (`sindri/primitives/process.h`)

Three pre-built instances of the `snd_process_api_t` interface are exported globally. These pointers are typically assigned to `ctx.proc_api` during injection initialization.

| Symbol | Backend | Source |
|---|---|---|
| `snd_proc_win` | Win32 API (`OpenProcess`, `VirtualAllocEx`, `WriteProcessMemory`, `VirtualProtectEx`, `CreateRemoteThread`, `CloseHandle`) | `src/primitives/process/win.c` |
| `snd_proc_nt` | NT API via PEB + EAT resolution (`NtOpenProcess`, `NtAllocateVirtualMemory`, `NtWriteVirtualMemory`, `NtProtectVirtualMemory`, `NtCreateThreadEx`, `NtClose`) | `src/primitives/process/nt.c` |
| `snd_proc_sys` | Direct syscalls via SSN resolution + ASM stub | `src/primitives/process/sys.c` |

---

## The Interface (`sindri/primitives/os_api.h`)

### `snd_process_api_t`

Function pointer table defining the remote process operations contract.

| Field | Signature | Description |
|---|---|---|
| `create_process_params` | `snd_status_t (*)(const void *nt_path_unicode, const wchar_t *cmd_line, PVOID *out_params)` | Allocates RTL process parameters (NT/syscall backends) |
| `free_process_params` | `snd_status_t (*)(PVOID params)` | Frees RTL process parameters |
| `create_process` | `snd_status_t (*)(const snd_process_api_t *api, const wchar_t *image_path, const wchar_t *command_line, HANDLE *out_process, HANDLE *out_thread)` | Creates a suspended process |
| `open_process` | `snd_status_t (*)(DWORD pid, DWORD desired_access, HANDLE *out_process)` | Opens a handle to the target process |
| `alloc_remote` | `snd_status_t (*)(HANDLE process, SIZE_T size, DWORD alloc_type, DWORD protect, PVOID *out_address)` | Allocates memory in the remote process |
| `write_remote` | `snd_status_t (*)(HANDLE process, PVOID base, const void *buffer, SIZE_T size, SIZE_T *bytes_written)` | Writes data into the remote process |
| `protect_remote` | `snd_status_t (*)(HANDLE process, PVOID base, SIZE_T size, DWORD new_protect, DWORD *old_protect)` | Changes memory protections in the remote process |
| `create_remote_thread` | `snd_status_t (*)(HANDLE process, PVOID start, PVOID param, HANDLE *out_thread)` | Creates a thread in the remote process |
| `close_handle` | `snd_status_t (*)(HANDLE handle)` | Closes a handle |

#### `create_process`

| Parameter | Description |
|---|---|
| `api` | Pointer to the owning `snd_process_api_t` (provides `create_process_params` and `free_process_params`) |
| `image_path` | Fully-qualified NT path to the target image |
| `command_line` | Command line to pass to the new process |
| `out_process` | Receives the process handle on success |
| `out_thread` | Receives the main thread handle (suspended) on success |

The NT and syscall backends build RTL process parameters via `create_process_params` and launch via `NtCreateUserProcess`; the Win32 backend uses `CreateProcessW`.

**Status codes:** `SND_STATUS_PROCESS_OPEN_FAILED`, `SND_STATUS_NULL_POINTER`

#### `open_process`

| Parameter | Description |
|---|---|
| `pid` | Target process ID |
| `desired_access` | Access mask (e.g. `PROCESS_ALL_ACCESS` or specific flags) |
| `out_process` | Receives the process handle on success |

**Status codes:** `SND_STATUS_PROCESS_OPEN_FAILED`, `SND_STATUS_NULL_POINTER`

#### `alloc_remote`

| Parameter | Description |
|---|---|
| `process` | Handle to the target process |
| `size` | Number of bytes to allocate |
| `alloc_type` | Win32 allocation flags (`MEM_COMMIT`, `MEM_RESERVE`) |
| `protect` | Initial page protection |
| `out_address` | Receives the remote base address on success |

All backends allocate at an OS-chosen base (`NULL` hint). The Win32 backend uses `VirtualAllocEx`; NT and syscall backends use `NtAllocateVirtualMemory`.

**Status codes:** `SND_STATUS_PROCESS_REMOTE_ALLOC_FAILED`, `SND_STATUS_NULL_POINTER`

#### `write_remote`

| Parameter | Description |
|---|---|
| `process` | Handle to the target process |
| `base_address` | Remote destination address |
| `buffer` | Local source buffer |
| `size` | Number of bytes to write |
| `bytes_written` | Optional; receives the number of bytes actually written |

**Status codes:** `SND_STATUS_PROCESS_REMOTE_WRITE_FAILED`, `SND_STATUS_NULL_POINTER`

#### `protect_remote`

| Parameter | Description |
|---|---|
| `process` | Handle to the target process |
| `base_address` | Remote base address |
| `size` | Region size |
| `new_protect` | Desired page protection |
| `old_protect` | Optional; receives the previous protection |

**Status codes:** `SND_STATUS_PROCESS_REMOTE_PROTECT_FAILED`, `SND_STATUS_NULL_POINTER`

#### `create_remote_thread`

| Parameter | Description |
|---|---|
| `process` | Handle to the target process |
| `start_address` | Remote thread entry point |
| `parameter` | Argument passed to the entry point |
| `out_thread` | Receives the thread handle on success |

NT and syscall backends use `NtCreateThreadEx` with `THREAD_ALL_ACCESS` (`0x1FFFFF`). The Win32 backend uses `CreateRemoteThread`.

**Status codes:** `SND_STATUS_THREAD_REMOTE_CREATE_FAILED`, `SND_STATUS_NULL_POINTER`

#### `close_handle`

Closes a process or thread handle. The NT backend treats a `NULL` handle as success.

**Status codes:** `SND_STATUS_HANDLE_CLOSE_FAILED`

---

### Customizing process creation (`sindri/primitives/process.h`)

Two helper functions allow injecting custom RTL parameter builders while reusing the shared NT/syscall process creation logic:

```c
snd_status_t WINAPI snd_nt_create_process_custom(
    const wchar_t *image_path, const wchar_t *command_line,
    HANDLE *out_process, HANDLE *out_thread,
    snd_process_create_params_cb create_cb,
    snd_process_free_params_cb   free_cb);

snd_status_t WINAPI snd_sys_create_process_custom(
    const wchar_t *image_path, const wchar_t *command_line,
    HANDLE *out_process, HANDLE *out_thread,
    snd_process_create_params_cb create_cb,
    snd_process_free_params_cb   free_cb);
```

These are used internally by `snd_proc_nt` and `snd_proc_sys` and are exposed for advanced consumers that need to supply custom parameter construction callbacks (e.g., spoofed command lines, custom inherited handles).

**Source:** `src/primitives/process/nt.c`, `src/primitives/process/sys.c`

---

### `snd_thread_api_t`

Function pointer table defining the thread operations contract.

| Field | Signature | Description |
|---|---|---|
| `queue_apc` | `snd_status_t (*)(HANDLE thread, PVOID apc_routine, PVOID apc_argument)` | Queues an APC to the target thread |
| `resume_thread` | `snd_status_t (*)(HANDLE thread)` | Resumes the target thread |
| `suspend_thread` | `snd_status_t (*)(HANDLE thread)` | Suspends the target thread |
| `close_handle` | `snd_status_t (*)(HANDLE handle)` | Closes a handle |

---

## Injection Context (`sindri/injection/context.h`)

Process backends are injected into the injection context:

```c
typedef struct _snd_inj_ctx_t {
    DWORD  target_pid;
    HANDLE target_process;
    PVOID  remote_base;
    PVOID  remote_entry_point;
    SIZE_T remote_size;
    HANDLE remote_thread;
    PVOID  remote_arg;

    snd_inj_stage_t stage;

    const snd_buffer_t      *payload;
    const snd_process_api_t *proc_api;
    const snd_thread_api_t  *thread_api;
    const wchar_t           *target_image_path;
} snd_inj_ctx_t;
```

Call `snd_inj_cleanup(&ctx)` to close handles and reset state after an injection attempt.

See the [Injection Domain](injection/README.md) for the full stage machine and chain orchestration.

---

## Primitives: Syscalls


Public API in `include/sindri/primitives/syscalls.h`. Implementation: `src/primitives/execution/syscalls/`.

---

## Constants

| Macro | Value | Description |
|---|---|---|
| `SND_MAX_SYSCALLS` | `500` | Max cached exports in sort resolver |
| `SND_MAX_SYS_NAME_LEN` | `256` | Max normalized export name length |

Internal pipeline depth: **4 strategies** (`SND_MAX_INTERNAL_STRATEGIES` in `pipeline.c`).

---

## Core Data Structures

### `snd_syscall_entry_t`

| Field | Type | Description |
|---|---|---|
| `pAddress` | `PVOID` | Stub address (populated by scan resolver) |
| `dwHash` | `DWORD` | Target function hash |
| `wSystemCall` | `WORD` | Resolved SSN |
| `pSyscallAddr` | `PVOID` | Gadget address for indirect invocation (populated by gadget finder) |
| `pSpoofAddr` | `PVOID` | Gadget address for spoofed invocation (populated by spoof finder) |
| `dwSpoofFrameSize` | `DWORD` | Frame size of the spoofed function (populated by spoof finder) |

> [!NOTE]
> The sort resolver sets `wSystemCall` only. `_sys` backends use `entry.wSystemCall` for invocation.

### `snd_syscall_args_t`

Arguments for the syscall invoker (`snd_syscall_direct_invoke_asm` or `snd_syscall_indirect_invoke_asm`). On x64, `arg1` is moved to `R10` per the syscall convention; `arg2`–`arg4` use `RDX`/`R8`/`R9`; `arg5`–`arg11` are stack arguments.

| Field | Type | Description |
|---|---|---|
| `ssn` | `WORD` | SSN placed in `EAX`/`RAX` |
| `arg1`–`arg11` | `PVOID` | Syscall arguments |
| `sys_addr` | `PVOID` | Gadget address for indirect invocation; ignored by direct stub |
| `spoof_addr` | `PVOID` | Gadget address for spoofed invocation; ignored by other stubs |
| `spoof_frame_size` | `DWORD` | Frame size of the spoofed function; ignored by other stubs |

> [!NOTE]
> `sys_addr` is placed at the end of the struct to preserve memory offsets for args 1-11, ensuring backward compatibility with existing ASM stubs.

---

## Pipeline Configuration

### `snd_syscall_resolver_t`

```c
typedef snd_status_t (*snd_syscall_resolver_t)(
    DWORD func_hash,
    snd_syscall_entry_t *entry_out
);
```

### `snd_syscall_set_resolver`

Sets the **primary** strategy and **resets** the chain to a single entry.

```c
void snd_syscall_set_resolver(snd_syscall_resolver_t resolver);
```

**Returns:** `void`

---

### `snd_syscall_add_resolver`

Appends a fallback strategy. Evaluated in registration order after the primary.

```c
snd_status_t snd_syscall_add_resolver(snd_syscall_resolver_t resolver);
```

**Returns:** `SND_OK`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_SYSCALL_PIPELINE_EXHAUSTED` (chain full)

---



## Built-in Resolvers

### `snd_syscall_resolve_ssn_scan`

Stub-byte SSN extraction with neighbor fallback. See [engines.md](primitives/syscalls/engines.md).

```c
snd_status_t snd_syscall_resolve_ssn_scan(
    DWORD func_hash, snd_syscall_entry_t *entry_out);
```

**Source:** `src/primitives/execution/syscalls/resolvers/scan.c`

---

### `snd_syscall_resolve_ssn_sort`

EAT sort-table SSN derivation. See [engines.md](primitives/syscalls/engines.md).

```c
snd_status_t snd_syscall_resolve_ssn_sort(
    DWORD func_hash, snd_syscall_entry_t *entry_out);
```

**Source:** `src/primitives/execution/syscalls/resolvers/sort.c`

---

### `snd_syscall_set_invoker`

Sets the global syscall invoker function pointer.

```c
void snd_syscall_set_invoker(snd_syscall_invoker_t invoker);
```

Built-in options: `snd_syscall_direct_invoke_asm`, `snd_syscall_indirect_invoke_asm`.

**Source:** `src/primitives/execution/syscalls/pipeline.c`

---

### `snd_syscall_set_gadget_finder`

Sets the global gadget finder function pointer. Only required when using indirect invocation.

```c
void snd_syscall_set_gadget_finder(snd_syscall_gadget_finder_t finder);
```

Built-in: `snd_syscall_find_gadget_scan`.

**Source:** `src/primitives/execution/syscalls/pipeline.c`

---

### `snd_syscall_set_spoof_finder`

Sets the global spoof finder function pointer. Only required when using spoofed invocation.

```c
void snd_syscall_set_spoof_finder(snd_syscall_gadget_finder_t finder);
```

Built-in: `snd_syscall_find_spoof_scan`.

**Source:** `src/primitives/execution/syscalls/pipeline.c`

---

## Resolution & Execution

### `snd_syscall_resolve`

Walks the configured strategy chain; returns on first success.

```c
snd_status_t snd_syscall_resolve(DWORD func_hash, snd_syscall_entry_t *entry_out);
```

**Returns:** `SND_OK`, `SND_STATUS_NTDLL_NOT_INITIALIZED` (clean NTDLL base not configured), `SND_STATUS_RESOLVER_NOT_INITIALIZED` (empty chain), or `SND_STATUS_SSN_NOT_FOUND` (all strategies failed)

---

### `snd_syscall_direct_invoke_asm`

Architecture-specific ASM stub (`src/primitives/execution/syscalls/invokers/direct_x64.asm` or `direct_x86.asm`). Executes the `syscall` / `sysenter` / `int 2Eh` instruction inline.

```c
extern NTSTATUS snd_syscall_direct_invoke_asm(snd_syscall_args_t *args);
```

**Returns:** raw `NTSTATUS` from the kernel

---

### `snd_syscall_indirect_invoke_asm`

Architecture-specific ASM stub (`src/primitives/execution/syscalls/invokers/indirect_x64.asm` or `indirect_x86.asm`). Jumps to a legitimate `syscall; ret` gadget inside NTDLL. Requires `sys_addr` to be populated (via `snd_syscall_find_gadget_scan` or a custom gadget finder).

```c
extern NTSTATUS snd_syscall_indirect_invoke_asm(snd_syscall_args_t *args);
```

**Returns:** raw `NTSTATUS`, or `STATUS_INVALID_PARAMETER` (`0xC000000D`) if `sys_addr` is NULL

---

### `snd_syscall_spoofed_invoke_asm`

Architecture-specific ASM stub (`src/primitives/execution/syscalls/invokers/spoofed_x64.asm` or `spoofed_x86.asm`). Jumps to a legitimate `syscall; ret` gadget inside NTDLL and spoofs the return address using a JMP-Trampoline inside a Fat Frame. Requires both `sys_addr` (via `snd_syscall_find_gadget_scan`) and `spoof_addr` / `spoof_frame_size` (via `snd_syscall_find_spoof_scan`).

```c
extern NTSTATUS snd_syscall_spoofed_invoke_asm(snd_syscall_args_t *args);
```

**Returns:** raw `NTSTATUS`, or `STATUS_INVALID_PARAMETER` (`0xC000000D`) if any required address is NULL

---

## Compile-Time Defaults (`SND_USE_DEFAULTS`)

| Macro | Default value (when enabled) |
|---|---|
| `SND_SYSCALL_INVOKER_DEFAULT` | `snd_syscall_indirect_invoke_asm` |
| `SND_SYSCALL_GADGET_FINDER_DEFAULT` | `snd_syscall_find_gadget_scan` |
| `SND_SYSCALL_SPOOF_FINDER_DEFAULT` | `snd_syscall_find_spoof_scan` |
| `SND_SYSCALL_RESOLVER_DEFAULT` | `snd_syscall_resolve_ssn_scan` |

When `SND_USE_DEFAULTS` is disabled (default), all four macros expand to `NULL`.

> [!TIP]
> **OpSec Rationale:** Why use a compile-time macro instead of just initializing the variables to default pointers in `pipeline.c`?
> If the variables were unconditionally initialized with pointers to `snd_syscall_indirect_invoke_asm` and `snd_syscall_find_gadget_scan`, the C linker would be forced to pull those entire functions (including the scanner logic and ASM stubs) into the final compiled binary, even if the user explicitly chose to use direct syscalls or no syscalls at all. 
> By using `SND_USE_DEFAULTS`, we ensure the default dependency graph is completely severed when disabled, keeping the payload footprint as lean and evasive as possible.

---

## Typical bootstrap (PoC pattern)

```c
PVOID ntdll = NULL;
snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);
snd_ntdll_set_clean(ntdll);
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);

snd_syscall_args_t args = {0};
// populate arguments as required (e.g. args.arg1 = ... )
NTSTATUS nt;
snd_status_t status = snd_syscall_invoke(SND_HASH_NTALLOCATEVIRTUALMEMORY, &args, &nt);
```

---

## Related documentation

- [Pipeline lifecycle](primitives/syscalls/pipeline.md)
- [Memory `_sys` backend](primitives/memory/internals.md)
- [Mapping `_sys` backend](primitives/mapping/internals.md)

---

