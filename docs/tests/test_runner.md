# Test Runner (`tests/loader/test_runner.py`)

**Location:** `tests/loader/test_runner.py`

Data-driven integration harness for the pe loading pipeline. Expands compact `Spec` objects across loader variants and architectures, then executes PoCs as subprocesses.

## Usage

```text
python tests/loader/test_runner.py [--corkami] [--mutate]
```

| Flag | Description |
|---|---|
| *(none)* | Core functional matrix + arch-mismatch guards |
| `--corkami` | Corkami PE fuzz corpus (requires extracted fixtures) |
| `--mutate` | Runtime PE mutation matrix via `pe_mutator.py` |

## Prerequisites

1. **Dual-arch builds** with debug output enabled (tests match stdout substrings):

   ```bash
   cmake -B build64 -A x64 -DSND_BUILD_TESTS=ON -DSND_BUILD_PAYLOADS=ON -DSND_ENABLE_DEBUG=ON -DSND_USE_PRINTF=ON
   cmake --build build64 --config Release
   cmake -B build32 -A Win32 -DSND_BUILD_TESTS=ON -DSND_BUILD_PAYLOADS=ON -DSND_ENABLE_DEBUG=ON -DSND_USE_PRINTF=ON
   cmake --build build32 --config Release
   ```

2. **Expected directory layout** (hardcoded in the runner):

   | Path | Contents |
   |---|---|
   | `build64/pocs/` | x64 `loader_winapi.exe`, `loader_nowinapi.exe`, `loader_coff.exe` |
   | `build32/pocs/` | x86 loader binaries |
   | `build64/tests/loader/` | x64 test DLLs/EXEs |
   | `build32/tests/loader/` | x86 test payloads |
   | `build64/tests/loaders/coff/` | x64 test BOFs |
   | `build32/tests/loaders/coff/` | x86 test BOFs |

   With `SND_ENABLE_DEBUG=OFF`, many `expect_stdout` checks fail because loader debug strings are stripped.

## Architecture

### Spec → TestCase expansion

Each `Spec` declares loader-agnostic intent:

| Field | Description |
|---|---|
| `loaders` | Which loader variants (`nowinapi`, `winapi`, `coff`) |
| `payload` | Fixture name under `tests/loader/` or `tests/loaders/coff/` |
| `export` | Optional DLL export for FFI bridge (PE) or BOF entry point (COFF) |
| `args` | Arguments passed to export |
| `expect_stdout` | Substring match on process stdout |
| `expect_retval` | Expected FFI return value |
| `expect_rc` | Expected process exit code |
| `expect_fail` | Loader should reject gracefully |

**Core matrix:** `len(SPECS) × len(LOADERS) × len(ARCHES)` — currently includes tests for both PE loaders and COFF loaders, plus arch-mismatch tests and optional Corkami/mutation suites.

### Test categories

#### 1. Functional tests (from `SPECS`)

End-to-end loader validation:

- DLL FFI argument passing (`SayHello`)
- Bad-args validation in payload
- Missing `-e` export CLI error
- Advanced DLL (imports, heap, dynamic load)
- Empty DLL (missing export directory)
- **VerifyInit** — DllMain + relocations
- **VerifyImports** — multi-import IAT resolution
- **VerifyTLS** — TLS callbacks before DllMain
- Basic EXE entry + exit code
- Advanced EXE (CRT init, heap, printf)

#### 2. Architecture mismatch

Feeds x86 payload to x64 loader (and vice versa); expects compatibility guard message from `snd_ldr_pe_compatibility_check`.

#### 3. Corkami fuzz (`--corkami`)

Feeds exotic PE fixtures from `tests/fixtures/pe/corkami/` to x64 `loader_winapi`. Requires extracting `corkami_fixtures.zip` (password: `infected`).

#### 4. PE mutation (`--mutate`)

Generates mutated PE variants at runtime; validates graceful reject or successful load. See [pe_mutator.md](pe_mutator.md).

> [!NOTE]
> Heaven's Gate is **not** covered by this runner — validate manually via `pocs/heavens_gate` on an x86 build under WoW64.

## Related documentation

- [Test payloads](test_payloads.md)
- [PE mutator](pe_mutator.md)
- [Building SindriKit](../getting_started/building.md)
