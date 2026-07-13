# Loaders Domain

The loaders domain maps and executes code payloads in memory. Each loader **technique** defines its own context type and stage machine; techniques are organized under subdirectories in `include/sindri/loaders/`.

Today **Reflective PE Loading** (`loaders/pe/`) and **COFF Object Loading** (`loaders/coff/`) are implemented. Additional techniques (e.g. module stomping, transacted hollowing) will add parallel subdirectories with their own context structures — unlike injection, loader contexts are **not** shared across techniques.

## Reflective vs. Injection

| Aspect | Loaders | Injection |
|---|---|---|
| Context type | Per-technique (`snd_ldr_pe_ctx_t`, `snd_ldr_coff_ctx_t`) | Shared (`snd_inj_ctx_t`) |
| Execution target | Local process (by default) | Remote process |
| Primary API table | `mem_api`, `mod_api` | `proc_api` |
| PE/COFF fixups | Full local pipeline | Local bake + remote write (`snd_inj_classic_pe`, `snd_inj_classic_coff`) |

The Classic injection chain (e.g. `snd_inj_classic_pe`, `snd_inj_classic_coff`) **reuses** the loader engine for local parse/reloc/import/symbol steps, then hands off to the injection domain for remote allocation and thread creation.

## Header Map

| Header | Role |
|---|---|
| `sindri/loaders.h` | Umbrella include (pe, coff) |
| `sindri/loaders/pe.h` | Reflective PE technique umbrella |
| `sindri/loaders/pe/engine.h` | `snd_ldr_pe_ctx_t`, per-stage engine |
| `sindri/loaders/pe/chain.h` | `snd_ldr_pe_prepare_image`, execute, detach |
| `sindri/loaders/coff.h` | COFF technique umbrella |
| `sindri/loaders/coff/engine.h` | `snd_ldr_coff_ctx_t`, per-stage engine |
| `sindri/loaders/coff/chain.h` | `snd_ldr_coff_load`, execute |

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
| `pocs/loader_coff/main.c` | `snd_ldr_coff_load`, `snd_ldr_coff_execute_image` | BOF execution with `snd_mem_sys` |

## Source map

| Source | Implements |
|---|---|
| `src/loaders/pe/chain.c` | `snd_ldr_pe_prepare_image`, `execute_image`, `detach_image` |
| `src/loaders/pe/engine.c` | Per-stage engine, section copy, page protection runs |
| `src/loaders/coff/chain.c` | `snd_ldr_coff_load`, `snd_ldr_coff_execute_image` |
| `src/loaders/coff/engine.c` | Section copy, relocations, symbol resolution (BOF) |

## Table of Contents

- [techniques.md](techniques.md) — Pipeline, stage machine, protection models for PE & COFF
- [api_reference.md](api_reference.md) — full API reference for PE & COFF loaders

## Planned techniques

```
include/sindri/loaders/
├── pe/             <- implemented (snd_ldr_pe_ctx_t)
├── coff/           <- implemented (snd_ldr_coff_ctx_t)
└── <technique>/    <- future, technique-specific context + engine + chain
```

Each technique owns its context struct, stage enum, engine functions, and chain orchestrator.

## Related documentation

- [Injection domain](../injection/README.md) — reuses loader engines for target baking
- [Examples](../../examples/README.md)

