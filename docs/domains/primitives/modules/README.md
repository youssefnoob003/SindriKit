# Module Primitives

DLL loading and symbol resolution in the **local** process. Operations route through an injected `snd_module_api_t` table with string-based and hash-based resolution.

There is **no** `snd_mod_sys` backend — import resolution uses PEB walking and EAT parsing (`snd_mod_nt`) even when memory and process ops use direct syscalls.

## Header map

| Header | Role |
|---|---|
| `sindri/primitives/modules.h` | `snd_mod_win`, `snd_mod_nt` |
| `sindri/primitives/os_api.h` | `snd_module_api_t` callback typedefs |

## Source map

| Source | Backend |
|---|---|
| `src/primitives/modules/win.c` | `snd_mod_win` |
| `src/primitives/modules/nt.c` | `snd_mod_nt` |

## Table of Contents

- [techniques.md](techniques.md) — Win32 vs NT resolution, hash-based lookup
- [api_reference.md](api_reference.md) — `snd_module_api_t` and instances

## Related documentation

- [Parsers: env](../../../parsers/env/README.md) — PEB module walking
- [Parsers: PE exports](../../../parsers/pe/README.md) — EAT resolution
- [Primitives domain](../README.md)
