# SindriKit Documentation

Technical documentation mirroring the framework layout: shared infrastructure at the base, architectural patterns above it, domain techniques and API references at the leaves.

## Table of Contents

### Introduction & onboarding
- [getting_started/](getting_started/) — CMake setup, DI bootstrap, first loader/injection workflow
- [examples/](examples/) — PoC walkthroughs across Win32, NT, and syscall profiles

### Core framework
- [architecture/](architecture/) — dependency injection, state machines, status tiers, implant integration
- [domains/](domains/) — primitives, loaders, injection, evasion (stub)

### Internal engines
- [parsers/](parsers/) — PE parsing and env/PEB introspection
- [common/](common/) — CRT-free helpers, buffers, hashing, status, debug

### Build & QA
- [config/](config/) — hash manifest (`config/hashes.ini`)
- [scripts/](scripts/) — pre-build automation (`generate_hashes.py`)
- [tests/](tests/) — integration test runner, PE mutator, fixtures

## Documentation conventions

| Page type | Typical sections |
|---|---|
| **README** (folder index) | Intro → header/source map → Table of Contents → Related documentation |
| **techniques.md** | Concepts, OpSec trade-offs, integration patterns |
| **api_reference.md** | Signatures, parameters, returns, source paths |

Syscalls use `pipeline.md` + `engines.md` instead of `techniques.md`. Execution splits FFI and Heaven's Gate into dedicated technique pages.

## Related documentation

Start here: [Getting started](getting_started/README.md) → [Basic usage](getting_started/basic_usage.md) → [Examples OpSec table](examples/README.md)
