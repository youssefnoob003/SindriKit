# Operational Domains

Actionable offensive capabilities grouped by function. Each domain is self-contained and interacts with others only through injected primitive interfaces — never through direct cross-domain imports.

> [!IMPORTANT]
> **Domain independence:** A loader must not call injection internals directly. Use documented chain APIs and shared primitive tables.

## Table of Contents

- [primitives/](primitives/README.md) — DI backends: memory, modules, process, mapping, syscalls, execution
- [loaders/](loaders/README.md) — In-memory payload bootstrapping (reflective PE implemented)
- [injection/](injection/README.md) — Remote process injection (classic shell + PE implemented)

## Related documentation

- [Architecture](architecture/README.md)
- [Examples](examples/README.md)
- [Getting started](getting_started/README.md)

- [Parsers](parsers/README.md)
- [Common](common/README.md)
- [Status Codes](status_codes.md)
- [Config](config/README.md)
- [Scripts](scripts/README.md)
- [Tests](tests/README.md)
