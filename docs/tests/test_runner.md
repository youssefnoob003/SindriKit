# Test Runner (`tests/loader/test_runner.py`)

**Location:** `tests/loader/test_runner.py`

The test runner is a data-driven integration test harness that validates the reflective loading pipeline end-to-end. It auto-generates concrete test cases from compact specifications and executes them against both loader variants across both architectures.

## Usage

```
python tests/loader/test_runner.py [--corkami]
```

| Flag | Description |
|---|---|
| *(none)* | Runs the core functional test matrix |
| `--corkami` | Enables the Corkami PE fuzz corpus tests (requires extracting `tests/fixtures/pe/corkami_fixtures.zip`) |

## Architecture

### The Spec → TestCase Expansion

Test definitions are written as compact `Spec` objects that are loader- and architecture-agnostic. Each `Spec` declares:

| Field | Description |
|---|---|
| `loaders` | Tuple of loader variants to test against (e.g., `("nowinapi", "winapi")`) |
| `payload` | Test payload name (e.g., `"test_dll"`, `"test_exe_advanced"`) |
| `export` | Optional DLL export name to invoke via the FFI bridge |
| `args` | Arguments passed to the export function |
| `expect_stdout` | Expected substring in the process stdout |
| `expect_retval` | Expected FFI return value (auto-formatted per architecture pointer width) |
| `expect_rc` | Expected process exit code |
| `expect_fail` | If `True`, the test expects the loader to reject the payload gracefully |

At runtime, each `Spec` is expanded across all `(loader × architecture)` combinations. With 9 specs × 2 loaders × 2 architectures, the core matrix produces 36 test cases from a single declarative table.

### Test Categories

#### 1. Functional Tests (from SPECS)
Validates the end-to-end loading pipeline:
- **DLL loading with correct arguments** — verifies FFI bridge argument passing (`SayHello` with 3 args).
- **DLL loading with wrong arguments** — validates the payload's own input validation.
- **Missing export parameter** — tests the CLI error path.
- **Advanced DLL** — exercises `kernel32.dll` imports, heap allocations, dynamic `LoadLibraryA`, and multi-API IAT resolution.
- **Empty DLL** — verifies graceful handling of a DLL with no export directory.
- **DllMain + Relocations** — confirms `DLL_PROCESS_ATTACH` fired and global variable relocations were applied.
- **TLS callbacks** — confirms the loader parsed `IMAGE_TLS_DIRECTORY` and fired callbacks before `DllMain`.
- **EXE loading** — validates entry point execution and exit code capture.
- **Advanced EXE** — exercises CRT initialization, heap allocation, and `printf`.

#### 2. Architecture Mismatch Tests
Feeds a 32-bit payload to a 64-bit loader (and vice versa) to verify the compatibility guard rejects it with a clear error message.

#### 3. Heaven's Gate Tests
- Runs the 32-bit PoC and expects WoW64 detection success.
- Runs the 64-bit PoC and expects a non-WoW64 rejection message.

#### 4. Corkami Fuzz Tests (opt-in via `--corkami`)
Iterates every `.exe` in `tests/fixtures/pe/corkami/` (a public corpus of exotic, standards-pushing PE files) and feeds each to the 64-bit `loader_winapi`. These tests verify that the parser does not crash on any real-world edge-case PE file.

#### 5. PE Mutation Tests (via `pe_mutator.py`)
Generates structurally mutated PE variants at runtime and validates that the loader either successfully loads benign mutations or gracefully rejects breaking mutations without crashing.
