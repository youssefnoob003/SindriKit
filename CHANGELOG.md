# Changelog

All notable technique additions, strategy improvements, and core architecture updates to SindriKit will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project attempts to adhere to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
