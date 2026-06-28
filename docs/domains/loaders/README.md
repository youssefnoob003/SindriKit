# Loaders Domain

The loaders domain maps and executes code payloads in memory. Each loader **technique** defines its own context type and stage machine; techniques are organized under subdirectories in `include/sindri/loaders/`.

Today only **Reflective PE Loading** is implemented (`loaders/reflective/`). Additional techniques (e.g. module stomping, transacted hollowing) will add parallel subdirectories with their own context structures — unlike injection, loader contexts are **not** shared across techniques.

## Reflective vs. Injection

| Aspect | Loaders | Injection |
|---|---|---|
| Context type | Per-technique (`snd_ldr_pe_ctx_t` today) | Shared (`snd_inj_ctx_t`) |
| Execution target | Local process (by default) | Remote process |
| Primary API table | `mem_api`, `mod_api` | `proc_api` |
| PE fixups | Full local pipeline | Local bake + remote write (`snd_inj_classic_pe`) |

The Classic PE injection chain (`snd_inj_classic_pe`) **reuses** the reflective loader engine for local parse/reloc/import steps, then hands off to the injection domain for remote allocation and thread creation.

## Header Map

| Header | Role |
|---|---|
| `sindri/loaders.h` | Umbrella include (reflective today) |
| `sindri/loaders/reflective.h` | Reflective technique umbrella |
| `sindri/loaders/reflective/engine.h` | `snd_ldr_pe_ctx_t`, per-stage engine |
| `sindri/loaders/reflective/chain.h` | `snd_ldr_pe_prepare_image`, execute, detach |

## KnownDlls Bootstrapping

KnownDlls mapping is **not** part of the loaders domain. It lives in the mapping/object-manager primitives:

- `snd_om_knowndll_map()` — `sindri/primitives/object_manager.h`
- Mapping backends — `snd_map_nt`, `snd_map_sys` — [mapping primitives](../primitives/mapping/techniques.md)

Loaders and injection PoCs use KnownDlls to bootstrap clean `ntdll` for syscall resolution before selecting `_sys` backends.

## PoCs

| PoC | Entry points | Profile |
|---|---|---|
| `pocs/loader_winapi/main.c` | `snd_ldr_pe_prepare_image`, `snd_ldr_pe_execute_image` | `snd_mem_win`, `snd_mod_win` |
| `pocs/loader_nowinapi/main.c` | same | `snd_mem_nt`, `snd_mod_nt` + syscall bootstrap |
| `pocs/loader_noCRT_nowinapi/main.c` | same | No CRT; `snd_mem_win`, `snd_mod_nt`, PEB `ntdll` bootstrap |

## Source map (reflective)

| Source | Implements |
|---|---|
| `src/loaders/reflective/chain.c` | `snd_ldr_pe_prepare_image`, `execute_image`, `detach_image` |
| `src/loaders/reflective/engine.c` | Per-stage engine, section copy, page protection runs |

## Table of Contents

- [techniques.md](techniques.md) — Reflective PE pipeline, stage machine, protection model
- [api_reference.md](api_reference.md) — full reflective loader API

## Planned techniques

```
include/sindri/loaders/
├── reflective/     ← implemented (snd_ldr_pe_ctx_t)
└── <technique>/    ← future, technique-specific context + engine + chain
```

Each technique owns its context struct, stage enum, engine functions, and chain orchestrator.

## Related documentation

- [Injection domain](../injection/README.md) — `snd_inj_classic_pe` reuses loader engine
- [Examples](../../examples/README.md)
