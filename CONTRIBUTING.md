# Contributing to SindriKit

SindriKit is a foundational C library for building offensive security capabilities. The core philosophy governing the entire project is: **"One abstraction layer. Every offensive domain."**

> [!WARNING]
> **Scope & Intent Restriction**
> SindriKit is an architectural framework, not a malware repository. Pull requests containing fully weaponized implants, ransomware, destructive capabilities, or hyper-specific vendor bypasses (e.g., "Bypass for Vendor X") will be immediately closed and locked. Contributions must be generalized, operational primitives (e.g., new parsing logic, generic injection techniques, execution agnostic wrappers).

Before contributing, you must familiarize yourself with the framework's architecture, primarily its strict separation of *Intent* from *Execution Mechanics*.

## Development Setup

SindriKit is designed **strictly for Windows targets** (`_WIN32` or `_WIN64`). The core build system relies on modern CMake (`>= 3.16`), MSVC, and Python 3 (for compile-time API string hashing).

1. Clone the repository.
2. Ensure you have **Visual Studio** (with C++ build tools), **CMake**, and a **Python 3** interpreter installed and available in your `PATH`.
3. Use the provided `build.bat` wrapper script at the repository root to configure and compile the project.

### Using `build.bat`

Do not manually invoke CMake for local development unless you are explicitly testing the `add_subdirectory()` integration. For standalone development, framework testing, and PoC building, always use `build.bat`. 

`build.bat` automatically configures and builds **both** x86 (`build32`) and x64 (`build64`) targets simultaneously, enforcing the `Release` configuration for MSVC compiler optimization:

```cmd
:: Build silently (production defaults: Release, SND_ENABLE_DEBUG=OFF)
build.bat

:: Build with debug prints via OutputDebugString (SND_ENABLE_DEBUG=ON)
build.bat debug

:: Build with printf to stdout instead of OutputDebugString
build.bat debug console

:: Change the compile-time string hashing algorithm
build.bat djb2
build.bat fnv1a

:: Build everything, including tests and PoC payloads (forces debug & console)
build.bat tests pocs
```

## Core Architectural Rules

I heavily scrutinize all pull requests against the following architectural constraints. Code that breaks these paradigms will not be merged.

### 1. The Dependency Injection (DI) Contract
Never hardcode Win32 or Native API calls inside the core logic of an offensive domain (e.g., inside the reflective loader or PE parser). 
- **Always** consume functions through injected API tables (e.g., `snd_memory_api_t`, `snd_module_api_t`).
- If you are building a new domain, your context structure (e.g., `snd_injection_ctx_t`) must accept these API tables so the operator can swap execution mechanics at runtime.

### 2. The Status System (`snd_status_t`)
Never return raw integers, `NULL` pointers, or standard `NTSTATUS`/`DWORD` error codes from a framework function.
- All fallible functions must return a `snd_status_t`.
- Utilize the framework macros: `SND_OK`, `SND_ERR()`, `SND_ERR_W32()`, `SND_ERR_CTX()`, and `SND_ERR_W32_CTX()`.
- Error handling must compile cleanly for both the `DEBUG` tier (verbose context strings) and the `SILENT` tier (zero strings). The macros handle this truncation transparently.

### 3. Compile-Time API Hashing
Never use plaintext API or module names as string literals in the C source code.
1. Add the required module and API names to `config/hashes.ini`.
2. The build system invokes `scripts/generate_hashes.py` during compilation to dynamically generate `generated/sindri_hashes.h` based on the active `SND_HASH_ALGO`.
3. Use the generated constants (e.g., `SND_HASH_NTDLL_DLL`, `SND_HASH_NTALLOCATEVIRTUALMEMORY`) in your code.

### 4. Zero Footprint (`SILENT` Tier)
> [!CAUTION]
> **No plaintext strings. No standard I/O.**
> When compiled via `build.bat` (which defaults to `SND_ENABLE_DEBUG=OFF`), the compiled binary must have **zero** framework-related `.rdata` string artifacts. 
- Do **not** use standard C library printing functions (`printf`) or Win32 debug APIs (`OutputDebugString`) directly.
- **Always** use the `SND_DEBUG_PRINT` macro, which safely compiles to nothing in production tiers.
- PRs that leak strings into the `SILENT` tier build will be instantly rejected.

### 5. Custom Assembly (MASM)
SindriKit utilizes custom MASM for FFI (`ffi_x64.asm`, `ffi_x86.asm`), cascading syscall resolution, and Heaven's Gate. If your feature requires low-level execution logic:
- You must provide both x86 and x64 implementations.
- You must adhere strictly to the respective calling conventions (e.g., 16-byte stack alignment and shadow space for Microsoft x64).

## Code Style & Warnings

> [!IMPORTANT]
> The build system enforces `/W4` and `/WX` (Warnings as Errors) on MSVC. 

- **C Standard**: C99 (`c_std_99`).
- Your code must compile **warning-free**. Pay extremely close attention to implicit truncations, signed/unsigned mismatches, padding anomalies, and pointer-size differences between 32-bit and 64-bit compilation.

## Testing Guidelines

SindriKit is a broad framework encompassing multiple offensive domains. While tests are **not strictly required** for every contribution, if you are adding a highly complex technique or a new parser, it is strongly encouraged to include a dedicated `tests` directory.

If your contribution requires rigorous validation, you can look to the existing PE loader's testing methodology (`tests/loader/`) as a structural example of how to build robust tests:

- **Declarative Execution Matrices**: Defining test specifications that automatically expand across architectures (`x86`/`x64`) and execution mechanics (e.g., testing both `winapi` and `nowinapi` profiles).
- **Dynamic Fuzzing & Mutation**: Programmatically altering inputs (as seen in `pe_mutator.py`) to simulate edge cases and ensure your implementation handles structural corruption without crashing.
- **External Corpuses & Architecture Guards**: Integrating external test suites (like Corkami) and explicitly guarding against execution mismatches.

While not all domains (e.g., simple memory primitives, injection techniques, or syscall strategies) require this level of stress testing, ensuring stability is a core tenet of the framework.

To execute the existing framework tests, you must first build the required binaries, then run the Python test harness:

1. **Build the Binaries**: The test suite requires the loader executables (source in `pocs/`) and the test payloads (source in `tests/loader/src/`). Passing the `tests` flag to the build script automatically compiles both the test payloads and the required PoC loaders.
   ```cmd
   build.bat tests
   ```
2. **Run the Matrix**: Execute the Python test runner to launch the testing matrix.
   ```cmd
   python tests/loader/test_runner.py --mutate --corkami
   ```

## Pull Request Process

1. Fork the repo and create a feature branch.
2. Implement your feature, adhering to the architecture.
3. If adding a new capability, update the corresponding markdown documentation in `docs/`.
4. Run `build.bat tests pocs` to ensure both x86 and x64 builds complete successfully and all tests pass.
5. **Squash your commits** into logical, atomic units before submitting.
6. Submit a PR outlining the objective, the approach, and validating its OpSec profile.
