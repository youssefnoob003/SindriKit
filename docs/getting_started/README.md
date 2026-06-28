# Getting Started

Onboarding guides for building SindriKit and wiring dependency-injection primitives into loaders, injection chains, and PoCs.

## Quick start

```bash
cmake -B build -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
build/pocs/loader_winapi/Release/loader_winapi.exe -f payload.dll -e DllRegisterServer
```

For CRT-less production builds: `SND_CRTLESS=ON` and `SND_ENABLE_DEBUG=OFF` — see [building.md](building.md).

## Table of Contents

- [building.md](building.md) — CMake integration, cache variables, compiler OpSec, DEBUG vs SILENT tiers
- [basic_usage.md](basic_usage.md) — include hierarchy, DI tables, syscall bootstrap, loader/injection workflows

## Reading order

1. [building.md](building.md) — compile `sindri::engine` and PoCs
2. [basic_usage.md](basic_usage.md) — `mem_api` / `mod_api` / `proc_api` and bootstrap
3. [Examples](../examples/README.md) — PoC walkthroughs
4. [Architecture](../architecture/README.md) — design patterns in depth

## Related documentation

- [Documentation root](../README.md)
- [Examples OpSec profiles](../examples/README.md)
