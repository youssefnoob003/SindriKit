# Env Parser

Runtime interpretation of Windows process environment structures — primarily the PEB and loader module lists. Headers under `include/sindri/parsers/env/`, aggregated by `include/sindri/parsers/env.h`.

NT layouts (`SND_PEB`, `SND_LDR_DATA_TABLE_ENTRY`, …) live in `include/sindri/internal/nt/peb.h`. The env parser uses these layouts without Win32 API calls.

## Header map

| Header | Role |
|---|---|
| `peb.h` | Local PEB access, module list walking (string and hash), process parameters / command line, WoW64 TEB probe |
| `ntdll.h` | Active/clean NTDLL registration and export lookup used by the syscall pipeline |

## Source map

| Source | Implements |
|---|---|
| `src/parsers/env/peb.c` | PEB module resolution, process parameters and command line |
| `src/parsers/env/ntdll.c` | Active/clean NTDLL state encapsulation |

## Scope

| Area | Status | Notes |
|---|---|---|
| PEB module walking | **Implemented** | `snd_peb_get_module_base`, `snd_peb_get_module_base_hash` |
| Local PEB accessor | **Implemented** | `snd_peb_get_local` (segment register read; x64/x86/ARM64) |
| Process parameters | **Implemented** | `snd_env_get_process_params`, `snd_env_get_command_line` |
| WoW64 TEB probe | **Implemented** | `snd_env_get_wow32_reserved` (fs:[0xC0]) |
| NTDLL state | **Implemented** | `snd_ntdll_set_clean`, `snd_ntdll_get_active_export`, `snd_ntdll_get_clean_export` |
| TEB access | Planned | Thread Environment Block helpers beyond the WoW64 probe |

## Table of Contents

- [internals.md](internals.md) — PEB layout, module list walking, hash lookup
- [api_reference.md](../../api_reference.md) — full public env API

## Related documentation

- [Parsers domain](../README.md)
- [PE parser](../pe/README.md) — EAT parsing used with env resolvers
- [Internal NT layouts](../../api_reference.md#internal-nt-layouts)
