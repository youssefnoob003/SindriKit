# Core Architecture

Design patterns, execution models, and telemetry constraints governing SindriKit.

> [!CAUTION]
> **No direct OS calls in domain logic.** Host interaction goes through injected API tables or explicitly bootstrapped execution layers (syscall pipeline, FFI bridges).

## Framework layers

```
┌─────────────────────────────────────────────────────────────┐
│  Application / implant / PoC                                │
└───────────────────────────┬─────────────────────────────────┘
                            ▼
┌─────────────────────────────────────────────────────────────┐
│  Domains: loaders · injection · (future) evasion            │
└───────────────────────────┬─────────────────────────────────┘
                            ▼
┌─────────────────────────────────────────────────────────────┐
│  Primitives + execution (DI tables, syscalls, FFI)          │
└───────────────────────────┬─────────────────────────────────┘
                            ▼
┌─────────────────────────────────────────────────────────────┐
│  Parsers · Common                                           │
└─────────────────────────────────────────────────────────────┘
```

## Table of Contents

- [dependency_injection.md](dependency_injection.md) — `os_api.h` contracts and backend matrix
- [state_machines.md](state_machines.md) — loader and injection stage graphs
- [status_system.md](status_system.md) — `snd_status_t`, DEBUG vs SILENT tiers
- [redteam_integration.md](redteam_integration.md) — CMake embed, hash rotation, BYOM

## Related documentation

- [Getting started](../getting_started/README.md)
- [Domains](../domains/README.md)
- [Examples](../examples/README.md)
