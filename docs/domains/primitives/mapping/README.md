# Mapping Primitives

Section mapping: open a named section object, map a view into the current process, close the handle. Used for KnownDlls bootstrapping and file-backed section workflows.

Backends are injected as `snd_mapping_api_t`. Higher-level helper `snd_om_knowndll_map()` composes mapping callbacks into a KnownDlls workflow (`sindri/primitives/object_manager.h`).

> [!CAUTION]
> **`snd_map_win` limitation:** `OpenFileMappingW` accepts DOS paths or Win32 namespace names only — not `\KnownDlls\...`. Use `snd_map_nt` or `snd_map_sys` for Object Manager paths.

## Header map

| Header | Role |
|---|---|
| `sindri/primitives/mapping.h` | `snd_map_win`, `snd_map_nt`, `snd_map_sys` |
| `sindri/primitives/object_manager.h` | `snd_om_knowndll_map` |
| `sindri/primitives/os_api.h` | `snd_mapping_api_t` callback typedefs |

## Source map

| Source | Role |
|---|---|
| `src/primitives/mapping/win.c` | `snd_map_win` |
| `src/primitives/mapping/nt.c` | `snd_map_nt` |
| `src/primitives/mapping/sys.c` | `snd_map_sys` |
| `src/primitives/object_manager/knowndlls.c` | KnownDlls orchestration |

## Table of Contents

- [techniques.md](techniques.md) — backend comparison, KnownDlls bootstrap
- [api_reference.md](api_reference.md) — `snd_mapping_api_t`, instances, object manager

## Related documentation

- [Syscalls](../syscalls/README.md) — bootstrap after KnownDlls `ntdll` map
- [Primitives domain](../README.md)
