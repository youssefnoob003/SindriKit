# Building SindriKit

SindriKit is built using modern CMake (`>= 3.16`). It supports building either as a standalone executable (for the PoCs) or seamlessly integrating as a static library target (`sindri::engine`) into existing C2 implants and malware staging toolchains.

The framework is strictly tailored for Windows targets (`_WIN32` or `_WIN64`), aggressively enforcing compiler warnings to prevent unintended implicit type conversions or padding behaviors that often lead to subtle footprint anomalies in memory.

## 1. CMake Integration

To embed SindriKit into a custom loader or implant workflow, rewriting the build system is unnecessary. SindriKit exposes `snd_engine` (aliased as `sindri::engine`), which handles all pre-build Python-based API hashing algorithms and architecture-dependent ASM compilation internally.

Drop the SindriKit repository into the project directory (e.g., `vendor/SindriKit`) and integrate it via `add_subdirectory()`:

```cmake
# In your parent project's CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(AdvancedImplant C CXX ASM_MASM)

# 1. Configure SindriKit Build Options (Optional but Recommended)
set(SND_BUILD_PAYLOADS OFF CACHE BOOL "Disable standalone PoCs")
set(SND_ENABLE_DEBUG OFF CACHE BOOL "Disable verbose tracking for production builds")
set(SND_HASH_ALGO "DJB2" CACHE STRING "Hashing algorithm for API resolution (DJB2, FNV1A)")

# 2. Add SindriKit subdirectory
add_subdirectory(vendor/SindriKit)

# 3. Link your implant against the SindriKit engine
add_executable(implant src/main.c)
target_link_libraries(implant PRIVATE sindri::engine)
```

### Build Configuration Variables

SindriKit is controlled by several CMake cache variables. The build system is designed to forcefully guard against breaking configurations (e.g., combining CRT-less requirements with standard library diagnostics).

| Variable | Default | Description | Constraints & Side Effects |
|---|---|---|---|
| `SND_ENABLE_DEBUG` | `OFF` | Enables verbose tracking, state machine prints, and line-level error contexts. | If `ON`, injects `<stdio.h>` and `printf`/`OutputDebugStringA` into `snd_status_t`. |
| `SND_USE_PRINTF` | `OFF` | Reroutes debug output from `OutputDebugStringA` to `printf`. | Only effective if `SND_ENABLE_DEBUG=ON`. Requires CRT. |
| `SND_CRTLESS` | `OFF` | Builds the engine without standard library dependencies, injecting compiler-intrinsic fallbacks (`memcpy`, `memset`). | **Forcefully disables** `SND_ENABLE_DEBUG` and `SND_USE_PRINTF` if enabled to prevent CRT linking. If `SND_BUILD_TESTS` is `ON`, `SND_CRTLESS` is forcefully disabled. |
| `SND_HASH_ALGO` | `DJB2` | The algorithm used for compile-time API resolution hashing. | Valid options: `DJB2`, `FNV1A`. |
| `SND_RANDOMIZE_SEED` | `OFF` | Randomizes the 32-bit compile-time global hash seed to thwart static analysis rainbow tables. | If `OFF`, uses a deterministic seed (`0x5381`) to prevent constant recompilation of dependent `.c` files during development. |
| `SND_BUILD_PAYLOADS` | `OFF` | Compiles the bundled `loader_winapi` and `loader_nowinapi` executables. | Used primarily for PoC verification. |
| `SND_BUILD_TESTS` | `OFF` | Compiles the internal test suite and PE mutator binaries. | **Requires CRT**. Forcefully sets `SND_CRTLESS=OFF`. |


### Pre-Build Code Generation (Algorithm Agility)
By default, the framework runs `scripts/generate_hashes.py` at configure time against `config/hashes.ini` to pre-compute compile-time string hashes. The manifest groups API names under `[module::<dll_name>]` sections. The build system automatically generates a hash for both the module and every API listed beneath it. Swapping the hashing algorithm (e.g., from `DJB2` to `FNV1A`) across the entire codebase is a single `SND_HASH_ALGO` CMake variable. This guarantees zero source changes during hash rotation. Furthermore, by passing `SND_RANDOMIZE_SEED=ON`, a totally fresh 32-bit mathematical seed is embedded alongside the hashes, ensuring static footprints continuously rotate.

## 2. Compiler Configuration and OpSec

SindriKit aggressively forces strict compilation. If an operation causes compiler ambiguity, it is treated as a fatal error.

### MSVC Optimization and Footprint
When building with Microsoft Visual C++ (MSVC), SindriKit enforces `/W4` and `/WX` (warnings as errors). For production builds, this should be coupled with optimization flags to minimize the binary footprint and strip debug artifacts:

```cmake
# Recommended MSVC Release Flags for your implant
if(MSVC)
    target_compile_options(implant PRIVATE
        /O1                     # Optimize for size
        /GS-                    # Disable buffer security checks (reduces imports/CRT dependency)
        /GR-                    # Disable RTTI
        /Zc:threadSafeInit-     # Disable magic statics (CRT dependency)
    )
    
    # Strip PDB paths and enforce dynamic base
    target_link_options(implant PRIVATE
        /PDBALTPATH:%_PDB%      # Strips local PDB path from PE headers
        /DYNAMICBASE            # Enforce ASLR
        /NXCOMPAT               # Enforce DEP
        /ENTRY:main             # Avoid CRT entrypoint if using pure C (Optional)
    )
endif()
```

> [!WARNING]
> **CRT Independence & Telemetry Leakage**
> If the CRT is stripped (`/ENTRY:main` or `/NODEFAULTLIB`), the implant must avoid standard C library functions (`malloc`, `printf`, `strcmp`). SindriKit natively supports CRT independence. This is achieved by implementing compiler-intrinsic fallbacks (like `memcpy` and `memset`) within `src/common/crt_manifest.c` and exposing safe string/memory utilities in `include/sindri/common/helpers.h`. 
> 
> However, there is a critical consequence: if `SND_ENABLE_DEBUG` or `SND_USE_PRINTF` are enabled, the `snd_status_t` layer will automatically pull in `<stdio.h>` and `printf` to output verbose error contexts. This breaks CRT-less compilation and silently re-introduces telemetry surfaces. Always compile using the `SILENT` tier (`SND_ENABLE_DEBUG=OFF`) for production to guarantee absolute CRT independence.

## 3. Build Tiers

SindriKit supports two build tiers controlled by the `SND_ENABLE_DEBUG` flag:

- **DEBUG Tier (`SND_ENABLE_DEBUG=ON`)**: Emits the `SND_DEBUG=1` definition. Used during local development. Outputs state machine transitions, parsed PE field values, and syscall resolution failures to `stdout`/`stderr`.
- **SILENT Tier (`SND_ENABLE_DEBUG=OFF`)**: Emits `SND_DEBUG=0`. Strips all string literals associated with error logging and context tracking out of the `.rdata` section, ensuring zero console output and a significantly reduced artifact footprint on disk.

> [!CAUTION]
> The `SILENT` tier (`SND_ENABLE_DEBUG=OFF`) is the **only** acceptable configuration for compiling production artifacts destined for target environments.
