# Changelog

All notable technique additions, strategy improvements, and core architecture updates to SindriKit will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project attempts to adhere to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.0.0] - 2026-06-22

SindriKit is a Windows evasion toolkit written in C. This first release provides the core engine, focusing on a Dependency Injection architecture that separates offensive techniques from underlying OS execution mechanics. By programming against a unified API abstraction, operators can switch execution profiles (e.g., standard Win32 APIs vs. direct syscalls) without modifying their core payload logic.

### Core Features (v1.0)

- **Syscall Resolution:** Dynamic SSN resolution with a cascading fallback pipeline supporting Hell's Gate, Halo's Gate, Tartarus' Gate, and VelesReek.
- **Kernel-State Bootstrapping:** Maps unhooked system modules directly from the `\KnownDlls` Object Manager directory to provide clean execution bases.
- **Algorithm Agility:** Compile-time API hashing (DJB2 or FNV1A). Hashing algorithms can be swapped across the entire project via a single CMake variable.
- **PE Parser:** A custom, bounds-checked PE32/PE32+ parser with explicit state tracking (`is_mapped`) for handling raw and memory-mapped files safely.
- **Dynamic FFI:** Custom MASM assembly bridges for executing arbitrary functions while adhering to strict x64 and x86 calling conventions.
- **Reflective Loader (PoC Domain):** A fully functional 8-stage in-memory PE loader built on the framework, capable of executing entirely via direct syscalls with zero Win32 API surface.
- **State-Tracked Contexts:** Offensive operations are governed by discrete state machines, enabling tasks to be paused safely (e.g., for sleep obfuscation) and cleanly resumed.

### Build and OpSec Constraints

- **Strict Compilation:** The build system enforces `/W4 /WX` to catch implicit truncations and pointer mismatches at compile time.
- **SILENT Tier:** Compiling with `SND_ENABLE_DEBUG=OFF` removes all state-machine prints and error contexts, ensuring no framework strings end up in the `.rdata` section.
- **CRT Independence:** The `SND_CRTLESS=ON` flag builds the engine without the C Standard Library, relying on compiler-intrinsic fallbacks.
