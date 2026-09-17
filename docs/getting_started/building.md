# Building SindriKit

SindriKit builds with CMake `>= 3.16` on **Windows only** (`_WIN32` / `_WIN64`). The static library target is `sindri_engine` (alias `sindri::engine`).

## First build (PoCs)

```bat
build.bat pocs
```

`build.bat` configures and compiles **both** architectures in `Release`:

| Target | Path |
|---|---|
| x86 `unified` | `build32/pocs/Release/unified.exe` |
| x64 `unified` | `build64/pocs/Release/unified.exe` |
| Engine | `build32/Release/sindri_engine.lib`, `build64/Release/sindri_engine.lib` |

Equivalent raw CMake (one architecture and directory per configure):

```bash
cmake -B build -A x64 -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
# → build/pocs/Release/unified.exe
```

The payload option builds one unified command-line PoC rather than separate loader and injection executables.

### `build.bat` keywords

Keywords combine, e.g. `build.bat pocs crtless` or `build.bat tests clean`.

| Keyword | Effect |
|---|---|
| `pocs` | Build the `unified` PoC |
| `crtless` | `SND_CRTLESS=ON` (combine with `pocs`) |
| `tests` | Debug + console + `SND_BUILD_TESTS=ON` (implies `pocs`) |
| `debug` | `SND_ENABLE_DEBUG=ON` |
| `console` | `SND_USE_PRINTF=ON` (stdout instead of `OutputDebugStringA`) |
| `clean` | Delete `build32/` and `build64/` before compiling |
| `djb2` / `fnv1a` | Compile-time hash algorithm |
| `random` | Randomize the compile-time hash seed |
| `defaults` | `SND_USE_DEFAULTS=ON` |
| `morph` | Run the mutation engine (`SND_MORPH=ON`) |

CRT-less builds compile the same `unified` command implementation with a different frontend. The CRT-less target has no CRT, console output, or Windows SDK dependency; it reads the process command line through the PEB and dispatches the shared commands through native Sindri backends:

```bat
build.bat pocs crtless
```

or with CMake:

```bash
cmake -B build -DSND_CRTLESS=ON -DSND_ENABLE_DEBUG=OFF -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
# → build/pocs/Release/unified.exe
# → build/Release/sindri_engine.lib
```

See [Examples](../examples/README.md) for usage.

---

## CMake integration (implant embed)

```cmake
cmake_minimum_required(VERSION 3.16)
project(Implant C CXX ASM_MASM)

set(SND_BUILD_PAYLOADS OFF CACHE BOOL "")
set(SND_ENABLE_DEBUG OFF CACHE BOOL "")
set(SND_CRTLESS ON CACHE BOOL "")          # optional

add_subdirectory(vendor/SindriKit)

add_executable(implant src/main.c)
target_link_libraries(implant PRIVATE sindri::engine)
```

Include `sindri.h` or granular headers (`sindri/primitives.h`, etc.). Hash constants come from `#include <sindri_hashes.h>` (generated in the **build tree**, not the source tree).

---

## Cache variables

| Variable | Default | Description | Notes |
|---|---|---|---|
| `SND_ENABLE_DEBUG` | `OFF` | Verbose status context, `SND_DEBUG_PRINT`, stage traces | Pulls in `<stdio.h>` when ON |
| `SND_USE_PRINTF` | `OFF` | Route debug to `stdout` instead of `OutputDebugStringA` | Requires `SND_ENABLE_DEBUG=ON` and CRT |
| `SND_CRTLESS` | `OFF` | CRT manifest fallbacks; `/NODEFAULTLIB`-friendly | Force-disables DEBUG/PRINTF; switches `unified` to the SDK-free frontend/native backend profile |
| `SND_HASH_ALGO` | `DJB2` | Compile-time hash algorithm (`DJB2`, `FNV1A`) | Regenerates `sindri_hashes.h` at configure |
| `SND_RANDOMIZE_SEED` | `OFF` | Random `SND_HASH_SEED` per configure | OFF keeps deterministic hashes for faster rebuilds |
| `SND_BUILD_PAYLOADS` | `OFF` | Build `pocs/` executables | Builds `unified`; CRT-less mode selects its SDK-free frontend and native backend profile |
| `SND_BUILD_TESTS` | `OFF` | Build test payloads + integration harness inputs | **Requires CRT**; forces `SND_CRTLESS=OFF` |
| `SND_BUILD_UNIT_TESTS` | `OFF` | Build the host-side unit test binary (`snd_unit_tests`) + ctest | **Requires CRT**; forces `SND_CRTLESS=OFF`. Run with `ctest --test-dir <build> -C Release` |
| `SND_ENABLE_ASAN` | `OFF` | Instrument the engine (and `unified`/unit tests) with AddressSanitizer | Test payloads stay uninstrumented (they are reflectively loaded). MSVC switches to the dynamic CRT (`/MD`) |
| `SND_MORPH` | `OFF` | Enables the mutation engine | Polymorphic C/ASM mutations + struct shuffling. See [mutator.md](../scripts/mutator.md). |
| `SND_USE_DEFAULTS` | `OFF` | Pre-configure syscall invoker, gadget finder, and resolver globals | Defaults to indirect invoke + scan resolver + gadget scan. **OpSec note**: Left OFF by default so unused ASM stubs and scanners aren't linked into the final binary. |

### Guards (CMake)

- `SND_BUILD_TESTS=ON` / `SND_BUILD_UNIT_TESTS=ON` → `SND_CRTLESS` forced OFF
- `SND_CRTLESS=ON` → `SND_ENABLE_DEBUG` and `SND_USE_PRINTF` forced OFF
- `SND_USE_DEFAULTS=ON` → invoker = `snd_syscall_indirect_invoke_asm`, gadget finder = `snd_syscall_find_gadget_scan`, resolver = `snd_syscall_resolve_ssn_scan`
- Non-Windows configure → fatal error
- **ARM64** configure (`-A ARM64`) → fatal error; SindriKit targets x86 and x64 only

CI runs the unit tests and the loader integration matrices on `windows-latest` after `build.bat tests`, and the documentation audit (`scripts/audit_docs.py`) on Linux. An advisory AddressSanitizer run of the PE mutation matrix is also provisioned.

Sanitizers reuse the existing mutation matrix with the engine instrumented:

```bash
cmake -B build64 -A x64 -DSND_ENABLE_ASAN=ON -DSND_BUILD_TESTS=ON -DSND_BUILD_PAYLOADS=ON \
      -DSND_ENABLE_DEBUG=ON -DSND_USE_PRINTF=ON
cmake --build build64 --config Release
python tests/loaders/pe/test_runner.py --mutate
```

Use a **clean build directory** when switching between CRT-less and test builds.

---

## Pre-build generation

At configure time, CMake runs two Python scripts (requiring a local Python 3 interpreter):

1. **Hash generation**:
```text
python scripts/generate_hashes.py config/hashes.ini <build>/generated/sindri_hashes.h <ALGO> <RANDOMIZE>
```
Output: `${CMAKE_BINARY_DIR}/generated/sindri_hashes.h`, on the include path via `target_include_directories`. See [generate_hashes.md](../scripts/generate_hashes.md) and [hashes manifest](../config/hashes_manifest.md).

2. **Status code generation**:
```text
python scripts/generate_status_codes.py
```
Output: `docs/status_codes.md`, parsing all `SND_STATUS_*` codes directly from C headers into Markdown.

---

## Compiler configuration (MSVC)

SindriKit enforces `/W4` `/WX` on the engine. Recommended release flags for implants:

```cmake
if(MSVC)
    target_compile_options(implant PRIVATE
        /O1
        /GS-
        /GR-
        /Zc:threadSafeInit-
    )
    target_link_options(implant PRIVATE
        /PDBALTPATH:%_PDB%
        /DYNAMICBASE
        /NXCOMPAT
    )
endif()
```

> [!WARNING]
> **CRT independence & telemetry**
> CRT-less builds (`/NODEFAULTLIB`, direct entrypoint) must not use `malloc`, `printf`, or `strcmp`. SindriKit supplies `snd_memcpy` / `snd_memzero` via `memory.h` and `src/common/crt_manifest.c`.
>
> Enabling `SND_ENABLE_DEBUG` or `SND_USE_PRINTF` pulls `<stdio.h>` into status/debug paths and breaks CRT-less linking. Production implants: **`SND_ENABLE_DEBUG=OFF`**.

---

## Build tiers

| Tier | Setting | Effect |
|---|---|---|
| **DEBUG** | `SND_ENABLE_DEBUG=ON` | `SND_DEBUG=1` — file/line/context in `snd_status_t`, debug prints, stage strings |
| **SILENT** | `SND_ENABLE_DEBUG=OFF` | `SND_DEBUG=0` — integer-only status, stripped `.rdata` diagnostics |

SILENT is required for production artifacts.

---

## Integration tests layout

The Python test runner expects **dual-arch** MSVC output trees. `build.bat tests` produces exactly that layout (`SND_BUILD_TESTS=ON`, `SND_ENABLE_DEBUG=ON`, `SND_USE_PRINTF=ON`, plus `pocs`):

```bat
build.bat tests
```

| Path | Contents |
|---|---|
| `build64/pocs/` | x64 PoC binaries |
| `build32/pocs/` | x86 PoC binaries |
| `build64/tests/loaders/pe/` | x64 PE test payloads |
| `build32/tests/loaders/pe/` | x86 PE test payloads |
| `build64/tests/loaders/coff/` | x64 COFF test payloads |
| `build32/tests/loaders/coff/` | x86 COFF test payloads |

See [test_runner.md](../tests/test_runner.md).

---

## Related documentation

- [Getting started: basic usage](basic_usage.md)
- [Architecture: red-team integration](../architecture/redteam_integration.md)
- [Architecture: internal Windows boundaries](../architecture/internal_boundaries.md)
- [Common: CRT manifest](../common/infrastructure.md)
