# Common Infrastructure

Conceptual overview of the shared utilities in `include/sindri/common/`. For function signatures see [api_reference.md](../api_reference.md).

---

## Module layout (post-refactor)

The former monolithic `helpers.h` and `nt_defs.h` were split for clarity:

```
include/sindri/common/
├── macros.h      <- compiler / linkage macros (was part of helpers)
├── memory.h      <- bounds + memzero/memcpy + SND_PTR_ADD
├── string.h      <- ASCII + wide string helpers (new wide APIs)
├── buffer.h      <- tracked buffers
├── hash.h
├── debug.h       <- debug macros (was mixed into helpers)
└── opcodes.h

include/sindri/status/
├── core.h        <- snd_status_t, core codes, error macros
└── facility.h    <- facility identifiers and encoding helpers

include/sindri/internal/windows/
├── types.h       <- ABI types and SDK boundary
├── constants.h   <- PAGE_*, MEM_*, GENERIC_READ, DLL_PROCESS_* (shared by Win32 and NT)
├── image.h       <- structures shared by PE and COFF
├── pe.h          <- PE-specific image definitions
└── coff.h        <- COFF-specific definitions

include/sindri/internal/win32/
├── constants.h   <- Win32-only CreateFile/Heap flags (includes windows/constants.h)
└── api.h         <- native declarations for Win32-backed code

include/sindri/internal/nt/
├── base.h        <- NT structures, status helpers, Unicode initialization
├── file.h        <- I/O status and file-information layouts
├── process.h     <- process/thread access and NtCreateUserProcess layouts
├── api.h         <- Nt* function pointer types
└── peb.h         <- PEB / loader layouts
```

File acquisition is an OS capability, not common infrastructure:

```text
include/sindri/primitives/files.h
src/primitives/files/win.c
src/primitives/files/nt.c
src/primitives/files/sys.c
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

The file primitive tables load a full file into a tracked buffer. `snd_file_win` uses Win32 file APIs; `snd_file_nt` uses native NTDLL exports; and `snd_file_sys` invokes the configured syscall pipeline directly. The native and syscall variants use their corresponding memory backend for buffer allocation. Production implants can bypass file loading and provide pre-allocated buffers directly.

---

## Internal NT types (moved from common)

NT-specific constants and layouts do not live under `common/`. Key items in `internal/nt/base.h`:

| Symbol | Purpose |
|---|---|
| `SND_NT_SUCCESS(status)` | NTSTATUS success test |
| `SND_OBJ_CASE_INSENSITIVE` | Object Manager attribute flag |
| `SND_InitializeObjectAttributes` | Initialize `SND_OBJECT_ATTRIBUTES` |
| `snd_init_unicode_string` | Build `SND_UNICODE_STRING` from wide buffer + length |

Function typedefs (`SND_NtOpenSection_t`, etc.) are in `internal/nt/api.h`. Process/thread access and `NtCreateUserProcess` layouts are in `internal/nt/process.h`; PEB structures are in `internal/nt/peb.h`.

## Windows SDK boundary

The engine contains both SDK-backed Win32 implementations and SDK-free native/syscall implementations. SDK-backed source files opt in with `SND_USE_WINDOWS_SDK=1`; `internal/windows/types.h` then imports `<windows.h>` and the real Win32 declarations. Native sources omit the macro and use the project-owned ABI declarations.

This is a source-level build contract, not a replacement API: functions such as `CreateFileA`, `ReadFile`, `VirtualFree`, and `HeapAlloc` remain the actual Windows functions in SDK-backed files.
