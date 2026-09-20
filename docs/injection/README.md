# Injection Domain

Remote process injection with three execution techniques: classic remote-thread
execution, early-bird APC execution, and suspended-thread context hijacking.
Target, staging, and cross-process operations route through injected capability
tables.

## Shared context vs loader contexts

Unlike loaders, **all injection techniques share** `snd_inj_ctx_t` (`sindri/injection/common/context.h`). Stage machine, handles, remote fields, and injected API tables are shared across classic, APC, and hijack techniques.

Each technique adds engine functions and chains under a subdirectory but mutates the same context:

```
include/sindri/injection/
├── common/
│   ├── context.h       <- shared context and stages
│   ├── cleanup.h       <- best-effort handle/process cleanup
│   ├── target.h        <- open/create targets
│   ├── staging.h       <- remote alloc/write/protect
│   └── prepare.h       <- shared PE/COFF preparation
├── classic/
│   ├── engine.h        <- classic execution engine
│   └── chain.h         <- snd_inj_classic_shell, snd_inj_classic_pe, snd_inj_classic_coff
├── apc/
│   ├── engine.h        <- APC execution engine
│   └── chain.h         <- snd_inj_apc_shell, snd_inj_apc_pe, snd_inj_apc_coff
└── hijack/
    ├── engine.h        <- frame preparation + hijack execute
    └── chain.h         <- snd_inj_hijack_shell, snd_inj_hijack_pe, snd_inj_hijack_coff
```

Loader contexts are **per-technique** (`snd_ldr_pe_ctx_t` today).

## Header map

| Header | Role |
|---|---|
| `sindri/injection.h` | Umbrella include |
| `sindri/injection/common/context.h` | `snd_inj_ctx_t`, stages |
| `sindri/injection/common/cleanup.h` | `snd_inj_cleanup` |
| `sindri/injection/common/target.h` | Target acquisition operations |
| `sindri/injection/common/staging.h` | Remote allocation, write, and protection operations |
| `sindri/injection/common/prepare.h` | Shared PE/COFF preparation |
| `sindri/injection/classic.h` | Classic technique umbrella |
| `sindri/injection/classic/engine.h` | Classic execution |
| `sindri/injection/classic/chain.h` | `snd_inj_classic_shell`, `snd_inj_classic_pe`, `snd_inj_classic_coff` |
| `sindri/injection/apc.h` | APC technique umbrella |
| `sindri/injection/apc/engine.h` | APC execution |
| `sindri/injection/apc/chain.h` | `snd_inj_apc_shell`, `snd_inj_apc_pe`, `snd_inj_apc_coff` |
| `sindri/injection/hijack.h` | Hijack technique umbrella |
| `sindri/injection/hijack/engine.h` | Frame preparation + hijack execute |
| `sindri/injection/hijack/chain.h` | `snd_inj_hijack_shell`, `snd_inj_hijack_pe`, `snd_inj_hijack_coff` |

## Implemented techniques

| Technique | Chain | Payload |
|---|---|---|
| Classic shellcode | `snd_inj_classic_shell` | Raw buffer in `inj_ctx.payload` |
| Classic PE | `snd_inj_classic_pe` | Local bake + remote execute (requires `snd_ldr_pe_ctx_t`) |
| Classic COFF | `snd_inj_classic_coff` | Local bake + remote execute (requires `snd_ldr_coff_ctx_t`) |
| APC shellcode | `snd_inj_apc_shell` | Raw buffer in `inj_ctx.payload` |
| APC PE | `snd_inj_apc_pe` | Local bake + remote APC execute (requires `snd_ldr_pe_ctx_t`) |
| APC COFF | `snd_inj_apc_coff` | Local bake + remote APC execute (requires `snd_ldr_coff_ctx_t`) |
| Hijack shellcode | `snd_inj_hijack_shell` | Raw buffer in `inj_ctx.payload` |
| Hijack PE | `snd_inj_hijack_pe` | Local bake + context-hijack execute (requires `snd_ldr_pe_ctx_t`) |
| Hijack COFF | `snd_inj_hijack_coff` | Local bake + context-hijack execute (requires `snd_ldr_coff_ctx_t`) |

## PoCs

| PoC | Chain | Profile |
|---|---|---|
| `pocs/src/cmd_inject_classic.c` | `snd_inj_classic_*` | `--win`, `--nt`, or `--sys` backend |
| `pocs/src/cmd_inject_apc.c` | `snd_inj_apc_*` | Early-bird APC command |
| `pocs/src/cmd_inject_hijack.c` | `snd_inj_hijack_*` | Thread-hijack command |

## Table of Contents

- [internals.md](internals.md) — classic pipelines, stage machine, loader interop
- [api_reference.md](../api_reference.md) — context, engine, and chain API

## Related documentation

- [Loaders domain](../loaders/README.md) — local PE bake for classic PE path
- [Process primitives](../primitives/process/README.md)
- [Examples](../examples/README.md)
