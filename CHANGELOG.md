# Changelog

All notable technique additions, strategy improvements, and core architecture updates to SindriKit will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project attempts to adhere to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.2.0] - 2026-06-29

Third major release. The framework introduces indirect syscalls and significantly improves the execution pipeline's flexibility and operator experience.

### Major Additions
- **Indirect Syscalls**: Architecture updated to decouple SSN resolution from invocation. Supports switching between direct and indirect syscall invocation dynamically.
- **Gadget Finder (`snd_syscall_find_gadget_scan`)**: Dynamically locates the `syscall; ret` (x64) or OS-transition instructions (x86) from the natively loaded `ntdll.dll` in the PEB to properly masquerade the call stack.
- **x86/x64 Support**: Full indirect syscall assembly stubs for both architectures (`invoke_indirect_x64.asm`, `invoke_indirect_x86.asm`), including proper stack frame alignment and teardown.

### Architecture & Refactoring
- **Compile-Time Defaults (`SND_USE_DEFAULTS`)**: Added a CMake flag to pre-configure the pipeline's globals (invoker, gadget finder, and primary resolver). Implemented as a macro for OpSec to prevent linking unused scanner/ASM dependencies when disabled.
- **Pipeline Overhaul**: Replaced `snd_syscall_strategy_set` terminology with `snd_syscall_set_resolver` and added `snd_syscall_set_invoker` / `snd_syscall_set_gadget_finder` to manage the decoupled execution flow.
- **Documentation**: Extensive documentation overhaul across all primitives, examples, PoCs, and architecture files to reflect the new pipeline structure and OpSec considerations.

---

## [1.1.0] - 2026-06-26

Second major release. The framework grows from a reflective-loader-centric engine into a multi-domain toolkit with expanded primitives, reorganized headers, and comprehensive documentation.

### Major Additions
- **Injection Domain**: Introduced classic remote injection (`snd_inj_classic_pe`, `snd_inj_classic_shell`) driven by a shared `snd_inj_ctx_t` state machine.
- **Expanded Primitives**: Added Process and Mapping APIs (`snd_proc_*`, `snd_map_*`) and Object Manager support for `\KnownDlls\` bootstrapping.
- **Syscall Pipeline**: Introduced pluggable syscall strategies (`snd_syscall_resolve_ssn_scan`, `snd_syscall_resolve_ssn_sort`).
- **New PoCs**: Added `inject_shell` and `inject_pe` to demonstrate stealth remote injection profiles.

### Architecture & Refactoring
- **Parsers Restructuring**: Split parsing into `pe/` and `env/` (PEB walking) subdomains with a unified export resolver.
- **Syscall Subsystem**: Encapsulated NTDLL state; bootstrap now requires explicit `snd_syscall_set_ntdll` before direct syscall backends operate.
- **Common Infrastructure**: Split monolithic `helpers.h` and `nt_defs.h` into granular headers (`memory.h`, `string.h`, `debug.h`, `nt/types.h`, etc.) and added robust bounds checking.
- **Loader Enhancements**: Updated reflective loader to use `snd_ldr_pe_ctx_t` with explicit state tracking (`snd_pe_target_t`) and introduced new runtime FFI macros (`SND_CALL_EXPORT`).

### Removals & Deprecations
- Removed legacy named gate resolvers (Hell's Gate, Halo's Gate, Tartarus, VelesReek) in favor of generic SSN strategies.
- Removed monolithic flat headers (`helpers.h`, `nt_defs.h`) and legacy parsing code.
- Removed implicit backend symbols (`snd_mem_native`, `snd_mod_native`) to enforce explicit `_nt` and `_sys` profiles.

---

## [1.0.1] - 2026-06-22

### Fixed
- **Loaders**: Fixed an access violation in `native_load_library` caused by resolving `LdrLoadDll` from an unmapped/disk NTDLL image. The loader now strictly resolves via the active PEB to maintain loader lock integrity.

---

## [1.0.0] - 2026-06-22

SindriKit is a Windows evasion toolkit written in C. This first release provides the core engine, focusing on a Dependency Injection architecture that separates offensive techniques from underlying OS execution mechanics.

### Major Additions
- **Syscall Resolution**: Dynamic SSN resolution with a cascading fallback pipeline supporting Hell's Gate, Halo's Gate, Tartarus' Gate, and VelesReek.
- **Kernel-State Bootstrapping**: Maps unhooked system modules directly from the `\KnownDlls` Object Manager directory.
- **Reflective Loader**: A fully functional 8-stage in-memory PE loader built on the framework, capable of executing entirely via direct syscalls.
- **Algorithm Agility**: Compile-time API hashing (DJB2 or FNV1A) via CMake.
- **PE Parser**: Custom, bounds-checked PE32/PE32+ parser with explicit state tracking (`is_mapped`).
- **Dynamic FFI**: Custom MASM assembly bridges for executing arbitrary functions safely.
