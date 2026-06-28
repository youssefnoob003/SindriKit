# Common Infrastructure

Conceptual overview of the shared utilities in `include/sindri/common/`. For function signatures see [api_reference.md](api_reference.md).

---

## Module layout (post-refactor)

The former monolithic `helpers.h` and `nt_defs.h` were split for clarity:

```
include/sindri/common/
├── macros.h      ← compiler / linkage macros (was part of helpers)
├── memory.h      ← bounds + memzero/memcpy + SND_PTR_ADD
├── string.h      ← ASCII + wide string helpers (new wide APIs)
├── buffer.h      ← tracked buffers
├── hash.h
├── status.h
├── debug.h       ← debug macros (was mixed into helpers)
└── disk.h

include/sindri/internal/nt/   ← was sindri/internal/nt_defs.h
├── types.h       ← NT structs, SND_PAGE_SIZE, unicode init
├── api.h         ← Nt* typedefs
└── peb.h         ← PEB / LDR layouts
```

---

## CRT Independence

SindriKit compiles under `/NODEFAULTLIB` for minimal import footprint. CRT replacements live in **`memory.h`** and **`string.h`** (header-only inlines):

| CRT | SindriKit | Header |
|---|---|---|
| `memset` | `snd_memzero` | `memory.h` |
| `memcpy` | `snd_memcpy` | `memory.h` |
| `strnlen` | `snd_strnlen` | `string.h` |
| `strncpy` | `snd_strncpy` | `string.h` |
| `strncat` | `snd_strncat` | `string.h` |
| `strchr` | `snd_strnchr` | `string.h` |
| `strncmp` | `snd_strncmp` | `string.h` |
| `wcsnlen` | `snd_wcsnlen` | `string.h` |
| `wcsnicmp` | `snd_wcsnicmp` | `string.h` |
| `wcsncpy` | `snd_wcsncpy` | `string.h` |
| `wcsncat` | `snd_wcsncat` | `string.h` |
| `mbstowcs` (ASCII→wide) | `snd_ascii_to_wide` | `string.h` |

### CRT manifest (`src/common/crt_manifest.c`)

MSVC may emit implicit `memcpy`/`memset` calls (struct copies, zero-init). Under `/NODEFAULTLIB`, `crt_manifest.c` provides global `memcpy`/`memset` symbols for the linker.

> Do **not** call `memcpy`/`memset` explicitly in framework code — use `snd_memcpy` / `snd_memzero`.

> [!CAUTION]
> `SND_DEBUG=1` or `SND_USE_PRINTF=1` pulls in `<stdio.h>` / `<stdarg.h>`. Disable debug for `/NODEFAULTLIB` release builds.

---

## Buffer bounds tracking

Raw pointers are wrapped in `snd_buffer_t` (`data`, `size`, optional `free_routine`). Access validation is layered:

1. **`snd_memory_bounds_check(total, offset, size)`** — pure arithmetic (`memory.h`)
2. **`snd_buffer_bounds_check(buf, offset, size)`** — against a tracked buffer (`buffer.h`)
3. **`snd_memory_ptr_bounds_check(base, total, ptr, size)`** — arbitrary pointer in a region (`memory.h`)

All return `1` if in bounds, `0` otherwise (not Win32 `BOOL` typedef, but equivalent semantics).

The PE parser's `snd_pe_rva_to_ptr` and syscall neighbor scan use these checks before dereferencing.

### Polymorphic free

`snd_buffer_free` dispatches through `free_routine`:

| Callback | Backend |
|---|---|
| `snd_buffer_free_heap` | `HeapFree` |
| `snd_buffer_free_virtual` | `VirtualFree` |
| `snd_buffer_free_mapped` | `UnmapViewOfFile` |

---

## API hashing

Compile-time hashes are generated from `config/hashes.ini` into **`sindri_hashes.h`** in the CMake build directory (`${CMAKE_BINARY_DIR}/generated/`). Runtime functions in `hash.h`:

| Function | Input | Casing | Typical use |
|---|---|---|---|
| `snd_hash` | `const char*` | Sensitive | Export names (`NtAllocateVirtualMemory`) |
| `snd_hash_lower` | `const char*` | Lowercased | Import DLL names |
| `snd_hash_wide_lower` | `const wchar_t*` | Lowercased | PEB `BaseDllName` |

Algorithm and seed are configured via CMake (`SND_HASH_ALGO`, `SND_RANDOMIZE_SEED`). See [config/hashes manifest](../config/hashes_manifest.md).

---

## Status system

All fallible framework functions return `snd_status_t` with:

- `code` — `snd_status_code_t` enum (PE, loader, syscall, PEB, OS error ranges)
- `os_error` — captured `GetLastError()` or `NTSTATUS` at failure site

Convenience macros: `SND_SUCCEEDED(x)`, `SND_FAILED(x)`.

When `SND_DEBUG=1`, the struct also carries `file`, `line`, and a 128-byte `context` buffer populated by `SND_ERR_CTX` / `SND_ERR_W32_CTX` / `SND_ERR_NT_CTX`.

When `SND_DEBUG=0`, context formatting is stripped — only integers remain. See [status system architecture](../architecture/status_system.md).

---

## Debug output (`debug.h`)

| `SND_DEBUG` | `SND_USE_PRINTF` | Output |
|---|---|---|
| `0` | — | All debug macros compile to no-ops |
| `1` | `1` | `fprintf` to stdout/stderr |
| `1` | `0` | `OutputDebugStringA` via internal `vsnprintf` buffer |

- **`SND_DEBUG_PRINT(fmt, …)`** — default debug line
- **`SND_FDEBUG_PRINT(stream, fmt, …)`** — stream-specific (printf mode only)
- **`SND_FALLBACK_STR(s)`** — returns `s` in debug builds, `""` in release (strips stage-name strings from `.rdata`)
- **`snd_dump_hex`** — hex+ASCII dump (gated by `SND_DEBUG`)

---

## Disk I/O

`snd_disk_buffer_load` reads a full file into a heap buffer with `snd_buffer_free_heap`. Used by PoCs; production implants typically receive payloads over the network into pre-allocated buffers.

---

## Internal NT types (moved from common)

NT-specific constants and layouts no longer live under `common/`. Key items in `internal/nt/types.h`:

| Symbol | Purpose |
|---|---|
| `SND_NT_SUCCESS(status)` | NTSTATUS success test |
| `SND_PAGE_SIZE` | `0x1000` |
| `SND_OBJ_CASE_INSENSITIVE` | Object Manager attribute flag |
| `SND_InitializeObjectAttributes` | Initialize `SND_OBJECT_ATTRIBUTES` |
| `snd_init_unicode_string` | Build `SND_UNICODE_STRING` from wide buffer + length |

Function typedefs (`SND_NtOpenSection_t`, etc.) are in `internal/nt/api.h`. PEB structures are in `internal/nt/peb.h`.
