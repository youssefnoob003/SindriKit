# Changelog

All notable technique additions, strategy improvements, and core architecture updates to SindriKit will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project attempts to adhere to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [2.1.1] - 2026-09-17

Patch release fixing the `SND_ENABLE_ASAN` build introduced in 2.1.0 and constraining the CI integration matrix. No public API change.

### Added
- Integration runners accept `--arch x64|x86` (repeatable) and `--exclude-substr <text>` to scope a run.

### Fixed
- **`SND_ENABLE_ASAN` (MSVC):** `/fsanitize=address` was applied to MASM sources (warning A4018), and ASan's C5072 ("without debug information") was promoted to an error by `/WX`. ASan flags are now scoped to C and ASan builds emit `/Zi` (plus `/wd5072`).
- **`SND_ENABLE_ASAN` runtime:** the MSVC runtime override used the invalid value `MultiThreadedDLLDebug`; it is now the valid `MultiThreaded[Debug]DLL`.

### Changed
- **CI integration matrix:** the hosted Windows image resolves x86/WOW64 syscall SSNs differently, causing the x86 syscall specs to fail there. CI now runs x64 in full and x86 without the syscall backends; the full matrix still runs locally.

---

## [2.1.0] - 2026-09-17

Additive release. The public API is source-compatible with 2.0.0: new build options and an opt-in syscall cache were added, and several correctness bugs were fixed.

### Added
- **Syscall entry cache:** `snd_syscall_cache_enable()` memoizes resolved entries by function hash (disabled by default; bypassed for spoofed invocation). The PoCs expose it as `--sys-cache`.
- **Unit tests:** `tests/unit/` (`snd_unit_tests`, `SND_BUILD_UNIT_TESTS=ON`) covers status encoding, bounds/range helpers, string helpers, hashing, buffer lifecycle, parser rejection, and regressions for the guards fixed below.
- **AddressSanitizer option:** `SND_ENABLE_ASAN=ON` instruments the engine and its consumers while leaving reflectively-loaded test payloads uninstrumented.
- **Docs audit:** `scripts/audit_docs.py` verifies relative links, same-file anchors, balanced code fences, and that every documented `snd_*` identifier exists in the code tree.
- **CI:** `.github/workflows/tests.yml` runs the docs audit, the unit tests, and both loader integration matrices (`--mutate`) on Windows, plus an advisory ASan job. `requirements-dev.txt` pins `pefile`.

### Fixed
- **MSVC `SND_CHECK_NULL`:** the traditional MSVC preprocessor mis-expands the variadic argument-count selection, silently dropping the guard for 3-argument calls (e.g. `snd_om_knowndll_map`). The engine now compiles with `/Zc:preprocessor`.
- **Sort resolver:** guard the bubble sort against an empty table and bound the export-name copy, preventing an out-of-bounds table walk and a one-byte stack overflow on malformed NTDLL input.
- **Syscall pipeline:** `snd_syscall_invoke` rejects NULL `args`/`out_nt_status`; the invoker/gadget/spoof setters accept NULL to clear, matching their documented contract.

### Changed
- **PoC architecture:** a `unified_backend_t` (`pocs/src/backend.c`) binds the file/memory/module/process/thread tables once; shared `-a` parsing and usage helpers replace per-command duplication. Removed the unused `load_clean_ntdll` helper and the `syscall_cfg` module.
- **Build:** ARM64 configuration now fails fast (x86/x64 only); `build.bat tests` also builds and runs the unit tests; `generate_status_codes.py` adds a decimal column to `docs/status_codes.md`.

### Documentation
- Reorganized `docs/`: examples rewritten around the `unified` CLI, consistent per-domain README/`internals` pages, corrected parser/loader boundaries, and a rewritten architecture set and API reference (all links, anchors, and identifiers audited).
- Updated the root `README`, `SECURITY`, and `CONTRIBUTING` to match current capabilities and commands.

---

## [2.0.0] - 2026-09-14

First major-version break since 1.x. Headers, CMake targets, and PoCs are not source-compatible with 1.6.0. The engine is reorganized around explicit Windows ABI layers, a facility-encoded status system, file I/O as a primitive, and a single `unified` PoC.

### Breaking Changes
- **Status headers:** `sindri/common/status.h` is gone. Include `sindri/status.h` (pulled in by `sindri.h`). Codes are facility-encoded (`SND_MAKE_STATUS`) with per-domain enums and `*_status_to_string` helpers under `include/sindri/status/`, plus domain `status.h` files for parsers, loaders, injection, primitives, and syscalls.
- **No implicit `<windows.h>`:** Public and native sources no longer include the SDK unconditionally. SDK-backed translation units opt in with `SND_USE_WINDOWS_SDK=1`. Shared scalars live in `internal/windows/types.h`.
- **Disk helper removed:** `common/disk.h` / `disk.c` are replaced by `snd_file_api_t` (`snd_file_win`, `snd_file_nt`, `snd_file_sys`).
- **Syscall pipeline API:** Resolvers no longer take an NTDLL base (`snd_syscall_resolver_t` is `(DWORD hash, snd_syscall_entry_t *)`). `snd_syscall_set_ntdll` is removed; clean/active NTDLL is owned by `parsers/env/ntdll.h` (`snd_ntdll_set_clean`, lazy PEB init). Call `snd_syscall_invoke` instead of duplicating resolve-then-asm at each backend.
- **PoC targets:** `loader_winapi`, `loader_nowinapi`, `loader_noCRT_nowinapi`, `loader_coff`, `inject_classic`, `inject_apc`, and `heavens_gate` executables are gone. `SND_BUILD_PAYLOADS` builds `unified` only (`unified load|inject|hg`). CRT-less mode uses `snd_crtless_poc_entry` and a PEB frontend, not `main`.
- **Internal NT split:** `internal/nt/types.h` is replaced by `nt/base.h`, `nt/file.h`, `nt/process.h`, `nt/peb.h`, and `nt/api.h`.

### Major Additions
- **File primitive:** Bounded load-into-buffer via Win32, NTDLL, or syscalls (`NtCreateFile` / `NtReadFile` / `NtQueryInformationFile` hashes).
- **NTDLL parser:** Dedicated env parser for active PEB NTDLL and an optional clean mapped image used by SSN/gadget scans.
- **Facility status engine:** `src/status/` implements generic, CLI, and file facilities; domains own the rest. `snd_status_print` / `snd_status_to_string` dispatch by facility.
- **Shared opcode table:** `common/opcodes.h` for syscall/gadget scanners and relocation math.
- **Native process creation:** NT/syscall backends use `NtCreateUserProcess` and RTL process-parameter helpers; `CreateProcessW` was dropped from the hash manifest.

### Architecture & Refactoring
- **Internal header layers:** `internal/windows/` (ABI types, `PAGE_*`/`MEM_*`, PE/COFF image layouts), `internal/win32/` (callable Win32 API + CreateFile/Heap flags), `internal/nt/` (native layouts and `Nt*` typedefs). See [internal_boundaries.md](docs/architecture/internal_boundaries.md).
- **Syscall sources:** `syscalls.c` / `*_scan.c` / `asm/invoke_*.asm` become `pipeline.c`, `resolvers/`, `finders/`, and `invokers/`.
- **PE sections:** `section_utils` renamed to `section`.
- **Win32 backends:** `*_win.c` (files, memory, mapping, modules, process, thread) are compiled only when `SND_CRTLESS` is off, and those TUs get `SND_USE_WINDOWS_SDK=1`.
- **CRT-less engine:** Defines `SND_CRTLESS=1`, keeps CRT manifest, and uses `/Gs2147483647` so MSVC does not emit `__chkstk`.
- **Documentation:** Examples, building, and architecture docs describe `unified`, the status facilities, and the Windows/Win32/NT split.

### Removals
- `include/sindri/common/status.h`, `src/common/status.c`, `include/sindri/common/disk.h`, `src/common/disk.c`
- Per-technique PoC trees under `pocs/`
- `snd_syscall_set_ntdll` and resolver signatures that threaded an NTDLL base

---

## [1.6.0] - 2026-08-16

Seventh major release. The framework introduces the Early Bird APC Injection technique, new thread-level primitives, and a consolidation of Proof-of-Concept implementations.

### Major Additions
- **APC Injection Engine (`snd_inj_apc_*`)**: Implemented the Early Bird APC injection technique. Supports queueing user APCs to a suspended process's main thread and resuming it. Features full support for Shellcode, PE, and COFF payloads via `snd_inj_apc_shell`, `snd_inj_apc_pe`, and `snd_inj_apc_coff`.
- **Thread Operations API (`snd_thread_api_t`)**: Added a new primitive interface for remote thread manipulation. Includes capabilities for queueing APCs (`queue_apc`), resuming (`resume_thread`), and suspending threads (`suspend_thread`).
- **Process Creation Primitive**: Expanded the `snd_process_api_t` interface with `create_process` to support creating suspended targets for APC injection.
- **Unified PoC Architecture**: Consolidated scattered injection examples into robust `inject_classic` and `inject_apc` tools. Both PoCs now feature a CLI that dynamically dispatches to Shellcode, PE, or COFF pipelines based on user arguments.

### Fixed
- **PE Parser Stability**: Fixed a critical unsigned integer underflow in `snd_pe_rva_to_ptr` that caused an out-of-bounds memory access if a virtual address did not map to any valid PE section.

## [1.5.0] - 2026-07-20

Sixth major release. The framework introduces the SindriKit Mutation Engine (`SND_MORPH`), a pre-build pipeline that injects structural and instruction-level polymorphism into the compiled binary dynamically.

### Major Additions
- **Mutation Engine Orchestrator (`SND_MORPH`)**: Added CMake integration that intercepts compilation, creating an ephemeral `morphed/` source tree where a series of Python passes randomly mutate the codebase before it is compiled. Generates a completely unique static binary signature on every build.
- **C Source Mutator (`junk_c.py`)**: Automatically parses C code to inject volatile-backed opaque predicates of varying shapes (simple `if`, opaque `while`, dummy `switch`, `do-while`). Includes a robust brace and semicolon parser to safely buffer statements and prevent `C4702` (unreachable code) errors.
- **Assembly Polymorphism (`masm_mutate.py`)**: Mutates x86 and x64 MASM stubs with functional NOP equivalents (e.g. `xchg eax, eax`) and mathematically neutral, EFLAGS-preserving operations (e.g. `lea reg, [reg]`, `mov reg, reg`). Fully ABI-aware to prevent register clobbering.
- **Structural Scrambling (`struct_shuffle.py`)**: Randomizes the memory layout of internal C structures dynamically. Supports opt-in via `SND_SHUFFLE_START`/`SND_SHUFFLE_END` macros, respects nested structures, and safely skips preprocessor blocks.
- **Call Graph Splitting & Dead Functions**: The C mutator can now dynamically generate random `static` C functions packed with non-signatured math loops at the global scope, and invoke them from within unreachable opaque predicate branches to severely complicate static call graph analysis.

## [1.4.0] - 2026-07-13

Fifth major release. The framework introduces COFF Object Loading and Injection, enabling stealthy execution of Beacon Object Files (BOFs) locally and remotely.

### Major Additions
- **COFF Loader Engine (`snd_ldr_coff_load`)**: Implemented a complete reflective loader for unlinked COFF object files. Supports zero-copy parsing, section allocation, and base relocation.
- **BOF Symbol Resolution (`MODULE$Func`)**: The engine dynamically resolves external OS APIs declared via the BOF standard convention by loading the target module and locating the export, dynamically building IAT entries and x64 trampolines on the fly.
- **Classic COFF Injection (`snd_inj_classic_coff`)**: A new high-level orchestrator that fuses a local COFF loading context with the shared remote injection context. The image is baked locally and marshaled across process boundaries alongside packed argument buffers for remote BOF execution.
- **COFF Parser Subdomain**: A new robust, bounds-checked parser (`include/sindri/parsers/coff/`) for COFF headers, section headers, symbol tables, and string tables.

### Architecture & Refactoring
- **Test Infrastructure**: Expanded the automated test runner to process `.obj` fixtures and added dedicated BOF integration payloads (`test_coff_msgbox.c`, `test_coff_args.c`) to validate external symbol resolution and argument parsing.

---

## [1.3.1] - 2026-07-07

OpSec Patch release addressing deterministic telemetry in stack spoofing.

### Security & OpSec
- **Fat Frame Entropy:** Implemented a pseudo-randomizer in `snd_syscall_find_spoof_scan` to prevent deterministic stack spoofing telemetry. The engine now uses the target function's SSN hash, a static counter, and the ASLR-dependent memory addresses of the `ntdll` base and the entry struct to randomize the selection of Fat Frames. This ensures payloads do not spoof the exact same `kernel32.dll` function on every execution, shifting the stack trace dynamically.
- **Fat Frame Hardening:** Added strict bounds checking to the spoof scanner. It now rejects functions that use a Frame Register (which could break `RtlVirtualUnwind` offset calculations) and caps the selected `frame_size` to a maximum of 240 bytes to ensure it cannot overflow the pre-allocated local stack space inside the MASM stub.

---

## [1.3.0] - 2026-07-06

Fourth major release. The framework introduces Call Stack Spoofing to defeat EDR virtual unwinding telemetry.

### Major Additions
- **Advanced Stack Spoofing (`snd_syscall_spoofed_invoke_asm`)**: Implemented full Call Stack Spoofing natively into the execution pipeline using a dynamic JMP-Trampoline.
- **Dynamic Fat Frame Discovery (`snd_syscall_find_spoof_scan`)**: Automatically parses the Exception Directory (`.pdata`) of `kernel32.dll` to locate functions with massive shadow stack allocations (>= 120 bytes). Embeds the syscall return gadget inside this frame, tricking `RtlVirtualUnwind` into parsing a perfectly intact, legitimate call chain without desynchronization.
- **Pipeline Overhaul**: Added `pSpoofAddr` and `dwSpoofFrameSize` to `snd_syscall_entry_t`. Added `snd_syscall_set_spoof_finder` to configure the spoof engine.

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
