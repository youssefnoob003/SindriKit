# Integration Tests

Automated testing infrastructure for validating the reflective loading pipeline and PE edge cases.

> [!NOTE]
> Tests require CRT (`SND_BUILD_TESTS=ON` forces `SND_CRTLESS=OFF`).

## Table of Contents

- [test_runner.md](test_runner.md) — data-driven runner, spec matrix, test categories
- [pe_mutator.md](pe_mutator.md) — PE mutation engine (benign and breaking cases)
- [test_payloads.md](test_payloads.md) — fixture payloads and validation logic

## Related documentation

- [Building SindriKit](../getting_started/building.md) — `SND_BUILD_TESTS` option
- [Loaders domain](../domains/loaders/README.md)
