# Loader Test Runners

The loader integration suites invoke the unified PoC rather than the deleted standalone loader executables.

**Locations:**

- PE: `tests/loaders/pe/test_runner.py`
- COFF: `tests/loaders/coff/test_runner.py`

Both runners expand compact specifications across backend variants and architectures, then execute `build{32,64}/pocs/Release/unified.exe`.

## Usage

```text
python tests/loaders/pe/test_runner.py [--corkami] [--mutate]
python tests/loaders/coff/test_runner.py
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
   | `build64/pocs/Release/unified.exe` | x64 unified PoC |
   | `build32/pocs/Release/unified.exe` | x86 unified PoC |
   | `build64/tests/loaders/pe/` | x64 test DLLs/EXEs |
   | `build32/tests/loaders/pe/` | x86 test payloads |
   | `build64/tests/loaders/coff/` | x64 test BOFs |
   | `build32/tests/loaders/coff/` | x86 test BOFs |

   With `SND_ENABLE_DEBUG=OFF`, many `expect_stdout` checks fail because loader debug strings are stripped.

## Architecture

### Spec → TestCase expansion

Each `Spec` declares backend-agnostic loader intent:

| Field | Description |
|---|---|
| `backends` | Backend profiles such as Win32, Native API, and syscall resolver/invoker combinations |
| `payload` | Fixture name under `tests/loaders/pe/` or `tests/loaders/coff/` |
| `export` | Optional DLL export for FFI bridge (PE) or BOF entry point (COFF) |
| `args` | Arguments passed to export |
| `expect_stdout` | Substring match on process stdout |
| `expect_retval` | Expected FFI return value |
| `expect_rc` | Expected process exit code |
| `expect_fail` | Loader should reject gracefully |

**Core matrix:** `len(SPECS) × len(BACKENDS) × len(ARCHES)` in each runner. The PE suite also includes architecture-mismatch tests and optional Corkami/mutation suites.

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

Feeds an x86 payload to the x64 unified PoC (and vice versa); expects the compatibility guard message from `snd_ldr_pe_compatibility_check`.

#### 3. Corkami fuzz (`--corkami`)

Feeds exotic PE fixtures from `tests/fixtures/pe/corkami/` to the runner's configured x64 PE loader. Requires extracting `corkami_fixtures.zip` (password: `infected`).

#### 4. PE mutation (`--mutate`)

Generates mutated PE variants at runtime; validates graceful reject or successful load. See [pe_mutator.md](pe_mutator.md).

> [!NOTE]
> Heaven's Gate is **not** covered by these loader runners — validate manually with `build32/pocs/Release/unified.exe hg` from an x86 build under WoW64.

## Related documentation

- [Test payloads](test_payloads.md)
- [PE mutator](pe_mutator.md)
- [Building SindriKit](../getting_started/building.md)
