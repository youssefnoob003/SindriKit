# Tests

Testing infrastructure: host-side unit tests plus loader integration runners.

> [!NOTE]
> Tests require the CRT (`SND_BUILD_TESTS=ON` / `SND_BUILD_UNIT_TESTS=ON` force `SND_CRTLESS=OFF`).

## Unit tests

`tests/unit/` builds `snd_unit_tests` (enabled by `SND_BUILD_UNIT_TESTS=ON`, included in `build.bat tests`) and covers the pure surface — status encoding, bounds helpers, string helpers, hashing, buffer lifecycle, and parser rejection paths. Run via ctest:

```sh
ctest --test-dir build64 -C Release --output-on-failure
```

## Integration runners

- [test_runner.md](test_runner.md) — data-driven runner, spec matrix, test categories
- [pe_mutator.md](pe_mutator.md) — PE mutation engine (benign and breaking cases)
- [test_payloads.md](test_payloads.md) — fixture payloads (PE DLL/EXE and COFF BOFs) and validation logic

CI (`.github/workflows/tests.yml`) runs the unit tests and both integration matrices on `windows-latest` after `build.bat tests`.

Injection is not covered by automated tests yet; validate `inject classic`/`inject apc` manually via the `unified` CLI (see [Examples](../examples/README.md)).

## Sanitizers

`SND_ENABLE_ASAN=ON` instruments the engine (and `unified`/unit tests) with AddressSanitizer. Test payloads are deliberately left uninstrumented because they are reflectively loaded rather than started by the OS loader. Run the existing mutation matrix under it to catch memory errors the functional runner cannot see:

```cmd
cmake -B build64 -A x64 -DSND_ENABLE_ASAN=ON -DSND_BUILD_TESTS=ON -DSND_BUILD_PAYLOADS=ON -DSND_ENABLE_DEBUG=ON -DSND_USE_PRINTF=ON
cmake --build build64 --config Release
python tests\loaders\pe\test_runner.py --mutate
```

CI runs an **advisory** ASan job (`.github/workflows/tests.yml`).

> [!NOTE]
> The mutation engine (`pe_mutator.py`) requires `pip install pefile` (`requirements-dev.txt`); without it the runner's `--mutate` mode is silently skipped.

## Related documentation

- [Building SindriKit](../getting_started/building.md) — `SND_BUILD_TESTS` / `SND_BUILD_UNIT_TESTS`
- [Loaders domain](../loaders/README.md)
