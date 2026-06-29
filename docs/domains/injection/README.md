# Injection Domain

Remote process injection: open target, allocate remote memory, write payload, set protections, create a remote thread. Cross-process operations route through an injected `snd_process_api_t` table.

## Shared context vs loader contexts

Unlike loaders, **all injection techniques share** `snd_inj_ctx_t` (`sindri/injection/context.h`). Stage machine, handles, remote fields, and `proc_api` are identical across classic and future techniques.

Each technique adds engine functions and chains under a subdirectory but mutates the same context:

```
include/sindri/injection/
├── context.h           <- shared across ALL techniques
└── classic/
    ├── engine.h        <- per-stage classic engine
    └── chain.h         <- snd_inj_classic_shell, snd_inj_classic_pe
```

Loader contexts are **per-technique** (`snd_ldr_pe_ctx_t` today).

## Header map

| Header | Role |
|---|---|
| `sindri/injection.h` | Umbrella include |
| `sindri/injection/context.h` | `snd_inj_ctx_t`, stages, `snd_inj_cleanup` |
| `sindri/injection/classic.h` | Classic technique umbrella |
| `sindri/injection/classic/engine.h` | Per-stage engine functions |
| `sindri/injection/classic/chain.h` | `snd_inj_classic_shell`, `snd_inj_classic_pe` |

## Implemented techniques

| Technique | Chain | Payload |
|---|---|---|
| Classic shellcode | `snd_inj_classic_shell` | Raw buffer in `inj_ctx.payload` |
| Classic PE | `snd_inj_classic_pe` | Local bake + remote execute (requires `snd_ldr_pe_ctx_t`) |

## PoCs

| PoC | Chain | Profile |
|---|---|---|
| `pocs/inject_shell/main.c` | `snd_inj_classic_shell` | KnownDlls bootstrap + `snd_proc_win` |
| `pocs/inject_pe/main.c` | `snd_inj_classic_pe` | `snd_mem_sys`, `snd_mod_nt`, `snd_proc_sys` |

## Table of Contents

- [techniques.md](techniques.md) — classic pipelines, stage machine, loader interop
- [api_reference.md](api_reference.md) — context, engine, and chain API

## Related documentation

- [Loaders domain](../loaders/README.md) — local PE bake for classic PE path
- [Process primitives](../primitives/process/README.md)
- [Examples](../../examples/README.md)
