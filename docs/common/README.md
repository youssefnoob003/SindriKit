# Common Infrastructure

Shared, domain-agnostic utilities under `include/sindri/common/`. Aggregated by `include/sindri/common.h`.

These headers provide CRT independence, bounded memory access, and hash-based API resolution without plaintext strings.

## Header map

| Header | Role |
|---|---|
| `macros.h` | `SND_FORCE_INLINE`, `SND_BEGIN_EXTERN_C`, linkage wrappers |
| `memory.h` | Bounds checks, `snd_memzero`, `snd_memcpy`, `SND_PTR_ADD` |
| `string.h` | Bounded ASCII and wide string operations |
| `buffer.h` | `snd_buffer_t` lifecycle and buffer bounds checking |
| `hash.h` | Runtime hashing (`snd_hash`, `snd_hash_lower`, `snd_hash_wide_lower`) |
| `debug.h` | `SND_DEBUG_PRINT`, `SND_FALLBACK_STR`, `snd_dump_hex` |
| `opcodes.h` | Shared opcode and instruction constants |

File loading (`snd_file_win` / `_nt` / `_sys`) is a primitive, not common infrastructure — see [files primitives](../primitives/files/README.md).

### Related: internal Windows headers

Windows ABI, Win32, and NT definitions live under `include/sindri/internal/` (not pulled in by `common.h`):

| Header | Role |
|---|---|
| `internal/windows/types.h` | Shared ABI types and SDK selection |
| `internal/windows/constants.h` | `SND_PAGE_*`, `SND_MEM_*`, `SND_GENERIC_READ`, `SND_DLL_PROCESS_*` |
| `internal/windows/pe.h` / `coff.h` | PE and COFF on-disk layouts |
| `internal/win32/api.h` | SDK-free Win32 function declarations |
| `internal/win32/constants.h` | Win32-only `OPEN_EXISTING`, `FILE_ATTRIBUTE_NORMAL`, `HEAP_ZERO_MEMORY` |
| `internal/nt/base.h` | `SND_UNICODE_STRING`, `SND_OBJECT_ATTRIBUTES`, `SND_NT_SUCCESS` |
| `internal/nt/file.h` | `SND_IO_STATUS_BLOCK` and file-information layouts |
| `internal/nt/process.h` | Process/thread access, `NtCreateUserProcess` structures and constants |
| `internal/nt/api.h` | `Nt*` / `LdrLoadDll` function pointer typedefs |
| `internal/nt/peb.h` | PEB, loader lists, process parameters layouts |
| `internal/nt.h` | NT umbrella include |

## Table of Contents

- [infrastructure.md](infrastructure.md) — CRT independence, bounds model, hashing pipeline, debug tiers
- [api_reference.md](../api_reference.md) — full public common API

## Related documentation

- [Architecture: status system](../architecture/status_system.md)
- [Config: hash manifest](../config/hashes_manifest.md)
