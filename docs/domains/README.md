# Operational Domains

Actionable offensive capabilities grouped by function. Each domain is self-contained and interacts with others only through injected primitive interfaces — never through direct cross-domain imports.

> [!IMPORTANT]
> **Domain independence:** A loader must not call injection internals directly. Use documented chain APIs and shared primitive tables.

## Table of Contents

- [primitives/](primitives/) — DI backends: memory, modules, process, mapping, syscalls, execution
- [loaders/](loaders/) — In-memory payload bootstrapping (reflective PE implemented)
- [injection/](injection/) — Remote process injection (classic shell + PE implemented)
- [evasion/](evasion/) — Planned: ETW/AMSI bypass, sleep obfuscation (stub)

## Related documentation

- [Architecture](../architecture/README.md)
- [Examples](../examples/README.md)
- [Getting started](../getting_started/README.md)
