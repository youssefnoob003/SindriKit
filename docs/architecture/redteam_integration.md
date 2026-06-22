# Redteam Integration

While SindriKit can be compiled standalone via `build.bat` (e.g., `build.bat tests pocs`) to test the included Proof of Concepts, its primary mandate is entirely different. 

SindriKit is built specifically to serve as an embedded library powering larger offensive toolsets, such as C2 implants, custom loaders, and malware staging pipelines containing an arsenal of techniques. By acting as the foundational engine, it allows operators to completely ignore the repetitive boilerplate of execution mechanics and low-level debugging. Instead, developers can focus purely on offensive intent while relying on the framework's internal `snd_status_t` system to handle verbose telemetry during development.

## CMake Integration

The primary method for integrating SindriKit is via CMake. By dropping the repository into the parent project and utilizing `add_subdirectory()`, operators can link the core execution primitives seamlessly.

```cmake
# Add SindriKit subdirectory
add_subdirectory(vendor/SindriKit)

# Link your implant against the SindriKit engine
add_executable(implant src/main.c)
target_link_libraries(implant PRIVATE sindri::engine)
```

By linking `sindri::engine`, the implant inherits all of SindriKit's memory primitives, syscall resolvers, PE parsers, and reflective loading capabilities without requiring re-implementation or complex build script maintenance.

## Algorithm Agility

SindriKit natively supports "Algorithm Agility", allowing operators to rapidly swap out underlying evasion techniques globally across the entire toolkit during the build process.

For instance, string hashing algorithms used for API resolution can be instantly changed via CMake variables:

```cmake
set(SND_HASH_ALGO "DJB2" CACHE STRING "Hashing algorithm for API resolution")
```

The framework automatically invokes Python scripts (`scripts/generate_hashes.py`) at configure time to recompute compile-time hashes for the selected algorithm. Crucially, when `SND_RANDOMIZE_SEED=ON` is provided, this script is also responsible for injecting a randomized compile-time hash seed for each build run. This ensures that even if the same algorithm is used, the static signature of the API resolution table shifts completely between compilations without altering a single line of C code.

## Bringing Your Own Mechanics (BYOM)

Thanks to the Dependency Injection architecture, integrating SindriKit into an established redteam workflow is frictionless. If an implant already possesses a highly sophisticated, custom evasion technique for allocating memory (e.g., via a signed driver or an exotic process hollowing technique), forking SindriKit is unnecessary.

An operator simply defines a `snd_memory_api_t` structure that points to the implant's proprietary functions, and injects it into the SindriKit loader context. SindriKit will utilize those custom mechanics to orchestrate its high-level operations.

## Silent Build Profiles

For production builds destined for target environments, SindriKit provides a SILENT build tier:

```cmake
set(SND_ENABLE_DEBUG OFF CACHE BOOL "Disable verbose tracking for production builds")
```

This enforces the `SND_DEBUG=0` macro, which comprehensively strips all string literals, error logs, and state tracking output from the final `.rdata` section. This guarantees zero console output and minimizes the static footprint of the injected engine.
