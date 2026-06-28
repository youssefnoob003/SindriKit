# Env Parser

Runtime interpretation of Windows process environment structures — primarily the PEB and loader module lists. Headers under `include/sindri/parsers/env/`, aggregated by `include/sindri/parsers/env.h`.

NT layouts (`SND_PEB`, `SND_LDR_DATA_TABLE_ENTRY`, …) live in `include/sindri/internal/nt/peb.h`. The env parser uses these layouts without Win32 API calls.

## Header map

| Header | Role |
|---|---|
| `peb.h` | Local PEB access, module list walking (string and hash) |

## Source map

| Source | Implements |
|---|---|
| `src/parsers/env/peb.c` | PEB module resolution |

## Scope

| Area | Status | Notes |
|---|---|---|
| PEB module walking | **Implemented** | `snd_peb_get_module_base`, `snd_peb_get_module_base_hash` |
| Local PEB accessor | **Implemented** | `snd_peb_get_local` (segment register read) |
| TEB access | Planned | Thread Environment Block helpers |
| Process parameters | Planned | Command line, image path, environment block |

## Table of Contents

- [techniques.md](techniques.md) — PEB layout, module list walking, hash lookup
- [api_reference.md](api_reference.md) — full public env API

## Related documentation

- [Parsers domain](../README.md)
- [PE parser](../pe/README.md) — EAT parsing used with env resolvers
- [Internal NT layouts](../../common/api_reference.md#internal-nt-layouts-includesindriinternalnt)
