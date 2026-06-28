# Building SindriKit

SindriKit builds with CMake `>= 3.16` on **Windows only** (`_WIN32` / `_WIN64`). The static library target is `sindri_engine` (alias `sindri::engine`).

## First build (PoCs)

```bash
cmake -B build -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
```

Outputs (MSVC multi-config):

| PoC | Path |
|---|---|
| `loader_winapi` | `build/pocs/loader_winapi/Release/` |
| `loader_nowinapi` | `build/pocs/loader_nowinapi/Release/` |
| `inject_pe` | `build/pocs/inject_pe/Release/` |
| `inject_shell` | `build/pocs/inject_shell/Release/` |
| `heavens_gate` | `build/pocs/heavens_gate/Release/` (requires **x86** / `-A Win32`) |

CRT-less single PoC:

```bash
cmake -B build -DSND_CRTLESS=ON -DSND_ENABLE_DEBUG=OFF -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
# → build/pocs/loader_noCRT_nowinapi/Release/
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
| `SND_CRTLESS` | `OFF` | CRT manifest fallbacks; `/NODEFAULTLIB`-friendly | Force-disables DEBUG/PRINTF; only builds `loader_noCRT_nowinapi` PoC |
| `SND_HASH_ALGO` | `DJB2` | Compile-time hash algorithm (`DJB2`, `FNV1A`) | Regenerates `sindri_hashes.h` at configure |
| `SND_RANDOMIZE_SEED` | `OFF` | Random `SND_HASH_SEED` per configure | OFF keeps deterministic hashes for faster rebuilds |
| `SND_BUILD_PAYLOADS` | `OFF` | Build `pocs/` executables | Full set when `SND_CRTLESS=OFF` |
| `SND_BUILD_TESTS` | `OFF` | Build test payloads + integration harness inputs | **Requires CRT**; forces `SND_CRTLESS=OFF` |

### Guards (CMake)

- `SND_BUILD_TESTS=ON` → `SND_CRTLESS` forced OFF
- `SND_CRTLESS=ON` → `SND_ENABLE_DEBUG` and `SND_USE_PRINTF` forced OFF
- Non-Windows configure → fatal error

Use a **clean build directory** when switching between CRT-less and test builds.

---

## Pre-build hash generation

At configure time, CMake runs:

```text
python scripts/generate_hashes.py config/hashes.ini <build>/generated/sindri_hashes.h <ALGO> <RANDOMIZE>
```

Output: `${CMAKE_BINARY_DIR}/generated/sindri_hashes.h`, on the include path via `target_include_directories`. See [generate_hashes.md](../scripts/generate_hashes.md) and [hashes manifest](../config/hashes_manifest.md).

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
> CRT-less builds (`/NODEFAULTLIB`, `/ENTRY:main`) must not use `malloc`, `printf`, or `strcmp`. SindriKit supplies `snd_memcpy` / `snd_memzero` via `memory.h` and `src/common/crt_manifest.c`.
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

The Python test runner (`tests/loader/test_runner.py`) expects **dual-arch** MSVC output trees:

| Path | Contents |
|---|---|
| `build64/pocs/` | x64 PoC binaries |
| `build32/pocs/` | x86 PoC binaries |
| `build64/tests/loader/` | x64 test payloads |
| `build32/tests/loader/` | x86 test payloads |

Configure with `SND_BUILD_TESTS=ON` and `SND_ENABLE_DEBUG=ON` (tests match stdout substrings from debug output). See [test_runner.md](../tests/test_runner.md).

---

## Related documentation

- [Getting started: basic usage](basic_usage.md)
- [Architecture: red-team integration](../architecture/redteam_integration.md)
- [Common: CRT manifest](../common/infrastructure.md)
