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
| `status.h` | `snd_status_t`, error macros, `SND_SUCCEEDED` / `SND_FAILED` |
| `debug.h` | `SND_DEBUG_PRINT`, `SND_FALLBACK_STR`, `snd_dump_hex` |
| `disk.h` | `snd_disk_buffer_load` (PoC file I/O) |

### Related: internal NT layouts

Windows NT structure definitions live in `include/sindri/internal/nt/` (not pulled in by `common.h`):

| Header | Role |
|---|---|
| `internal/nt/types.h` | `SND_UNICODE_STRING`, `SND_OBJECT_ATTRIBUTES`, `SND_NT_SUCCESS` |
| `internal/nt/api.h` | `Nt*` / `LdrLoadDll` function pointer typedefs |
| `internal/nt/peb.h` | PEB, loader lists, process parameters layouts |
| `internal/nt.h` | Umbrella include |

## Table of Contents

- [infrastructure.md](infrastructure.md) — CRT independence, bounds model, hashing pipeline, debug tiers
- [api_reference.md](api_reference.md) — full public common API

## Related documentation

- [Architecture: status system](../architecture/status_system.md)
- [Config: hash manifest](../config/hashes_manifest.md)
