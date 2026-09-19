<p align="center">
  <img src="assets/banner.png" width="100%">
</p>

<h1 align="center">SindriKit</h1>

<p align="center">
  <strong>Offensive Development Deserves Better Architecture.</strong><br>
  <em>A C library for building offensive capabilities.</em>
</p>

---

## Core Concept

Most offensive utilities hardcode their execution mechanics inside the technique's logic. A reflective loader doesn't just map an image; it maps it using a specific, hardcoded chain of `VirtualAlloc` or native NTAPI calls. When an EDR starts monitoring that specific chain, you are forced to rewrite the entire tool.

SindriKit solves this by enforcing a separation of concerns via interface abstraction tables:

1. **The Technique Logic:** (e.g., loaders, injections, patchers) deals with state tracking and data orchestration. It has no knowledge of how memory is allocated or how threads are created.
2. **The Execution Mechanics:** (e.g., Win32 API, Native NTAPI, Direct Syscalls) are inside independent API tables and injected into the technique at runtime.

By shifting execution mechanics to runtime function pointers, you can swap your entire strategy from Win32 calls to raw direct syscalls with a single line of code—without changing your payload execution logic.

---

## Design Architecture

* **Decoupled Execution Profiles:** Swap memory, module, mapping, process, thread, and file mechanics through independent function-pointer tables (`snd_memory_api_t`, `snd_module_api_t`, `snd_process_api_t`, `snd_thread_api_t`, `snd_mapping_api_t`, `snd_file_api_t`) without touching technique logic.
* **Technique Coverage:** Reflective **PE** (EXE/DLL) and **COFF/BOF** loading, **classic** and **early-bird APC** injection over shellcode/PE/COFF, plus architecture-aware FFI and Heaven's Gate — all over the same composable profiles.
* **Cascading Syscall Pipeline:** Pluggable SSN resolvers (`snd_syscall_resolve_ssn_scan`, `snd_syscall_resolve_ssn_sort`) with a priority chain, decoupled from invokers: **direct**, **indirect** (NTDLL gadget), or **spoofed** (dynamic Fat-Frame call-stack spoofing).
* **Facility-Encoded Status:** Every fallible call returns `snd_status_t` — a packed facility/local code plus the captured OS error, with context strings that compile out in the silent tier.
* **Compile-Time Obfuscation:** String and API hashing algorithms (DJB2, FNV1A) can be swapped globally via CMake. Compiling automatically randomizes the global seed to alter static signatures.
* **Mutation Engine:** Enables deep polymorphism via `SND_MORPH`. Generates unique binary signatures on every build by injecting volatile opaque predicates into C code, functionally equivalent math/NOPs into Assembly stubs, and scrambling the memory layout of core structs.
* **Release Builds:** A silent tier strips all diagnostic strings, file descriptors, and tracking frames; `SND_CRTLESS` builds go further with `/NODEFAULTLIB`, no SDK header, a PEB frontend, and native backends only.

---

## Quick Start

The repository ships a single `unified` CLI that exercises every profile:

```sh
build.bat pocs
build64\pocs\Release\unified.exe load pe -f payload.dll -e Run --sys
```

`unified` supports `load pe|coff`, `inject classic|apc|hijack` (shell, PE, COFF), and `hg`, each over `--win`/`--nt`/`--sys`. See [Examples & PoCs](docs/examples/README.md) and [Getting Started](docs/getting_started/README.md).

---

## Integrating SindriKit

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyTool C ASM_MASM)

set(SND_BUILD_PAYLOADS  OFF    CACHE BOOL   "")
set(SND_ENABLE_DEBUG    OFF    CACHE BOOL   "")
set(SND_HASH_ALGO      "DJB2"  CACHE STRING "")
set(SND_RANDOMIZE_SEED  ON     CACHE BOOL   "")
set(SND_MORPH           ON     CACHE BOOL   "")

add_subdirectory(libs/SindriKit)

add_executable(my_tool src/main.c)
target_link_libraries(my_tool PRIVATE sindri::engine)
```

```sh
cmake -B build && cmake --build build --config Release
```

Just two lines for your tool to inherit all of SindriKit's capabilities: PE and COFF parsing, reflective loading, cascading syscalls, and injection profiles.

---

## The Engine

### API Abstraction Layer

```
        ┌────────────────────────────────────────────────────────────────────────────┐
        │                          ANY OFFENSIVE INTENT                              │
        │    Loader · Injector · Spoofer · Patcher · Bypasser · Harvester · ...      │
        ├────────────────────────────────────────────────────────────────────────────┤
        │                     SINDRIKIT API ABSTRACTION LAYER                        │
        │      snd_memory_api_t   ->  alloc · free · protect                         │
        │      snd_module_api_t   ->  load_library · get_proc_address · ...          │
        │      snd_process_api_t  ->  open · alloc_remote · write · protect · thread │
        │      snd_mapping_api_t  ->  open · view · close   (KnownDlls bootstrap)    │
        │      snd_thread_api_t   ->  queue_apc · resume · suspend                   │
        │      snd_file_api_t     ->  load                                           │
        ├──────────────────┬──────────────────────┬──────────────────────────────────┤
        │   Win32 Profile  │    Native Profile    │    Bring Your Own Mechanic       │
        │  VirtualAlloc    │  NtAllocateVirtual   │  Driver · ROP · Exotic           │
        │  LoadLibraryA    │  PEB Walk + EAT      │  Operator-defined functions      │
        └──────────────────┴──────────────────────┴──────────────────────────────────┘
```

In practice, this means every domain follows the same contract:

```c
// Reflective loader
snd_ldr_pe_ctx_t ctx = {0};
ctx.raw_source = &payload;
ctx.mem_api    = &snd_mem_win;   // or snd_mem_nt / snd_mem_sys
ctx.mod_api    = &snd_mod_win;   // or snd_mod_nt
snd_ldr_pe_prepare_image(&ctx);
snd_ldr_pe_execute_image(&ctx);

// Classic injection
snd_inj_ctx_t inj = {0};
inj.target_pid = 1337;
inj.payload    = &shellcode;
inj.proc_api   = &snd_proc_sys;  // or snd_proc_win / snd_proc_nt
snd_inj_classic_shell(&inj);
snd_inj_cleanup(&inj);
```

### Cascading Syscall Pipeline

SindriKit treats syscall resolution as an injectable mechanic, stacking strategies in priority order. The engine falls through until one succeeds:

```c
snd_ntdll_set_clean(clean_ntdll);
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
snd_syscall_set_invoker(snd_syscall_direct_invoke_asm);
// or for indirect syscalls:
// snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
// snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);
// or for spoofed syscalls:
// snd_syscall_set_invoker(snd_syscall_spoofed_invoke_asm);
// snd_syscall_set_spoof_finder(snd_syscall_find_spoof_scan);
```

The invoker is decoupled from SSN resolution — switch between direct, indirect, and spoofed syscalls without modifying domain code. Indirect invocation jumps to a legitimate NTDLL gadget so the return address stays inside `ntdll.dll`; spoofed invocation additionally plants a genuine caller return address inside a dynamically discovered "Fat Frame", so call-stack unwinds stay coherent.

### Compile-Time Algorithm Agility

Every API name and module string is removed from the final binary at compile time via a single CMake variable:

```cmake
set(SND_HASH_ALGO "FNV1A")  # or DJB2 recomputes everything automatically
set(SND_RANDOMIZE_SEED ON)  # generates a fresh 32-bit seed on next configure
```

Each hash is computed with a randomly generated seed (if `SND_RANDOMIZE_SEED=ON`). Static footprint shifts completely between compilations without touching a line of C.

### Architecture-Aware Dynamic FFI

A custom MASM assembly bridge for arbitrary runtime function invocation. x64 builds follow the Microsoft x64 calling convention precisely (shadow space, register argument placement, stack alignment). x86 builds push arguments in reverse order with support for both `cdecl` and `stdcall` targets.

### Bounds-Checked PE Parser

A unified PE32/PE32+ parser with an `is_mapped` flag that correctly handles both raw on-disk images and memory-mapped views. Every data directory access is validated against tracked buffer bounds before dereferencing. Export resolution supports forwarder chains up to depth 4 with hash-based lookup.

Tested against:
- 40+ core test combinations targeting edge-case EXEs, DLLs, bad arguments, missing exports, and TLS callbacks across x86 and x64.
- 100+ dynamic PE mutations generated by the `pe_mutator` module: zeroed section names, integer overflows, invalid `e_lfanew` bounds, mangled imports.
- Full Corkami corpus: cleanly loads valid samples, cleanly rejects malformed ones without crashing across 99% of the samples.

### COFF / BOF Loader

A second loader technique handles unlinked COFF object files (Beacon Object Files): bounded parsing of headers, sections, symbols, and relocations; `MODULE$Function` external symbol resolution through the injected `mod_api`; x64 `JMP [RIP+0]` trampolines for out-of-range calls; and execution of a named entry point (default `go`) — locally or marshaled into a remote process.

### State-Tracked Domain Contexts

Every offensive operation is managed through a discrete context structure with stage enumeration. Operations can be paused between stages for sleep obfuscation or staged deployment, resumed cleanly, and inspected for the exact failure point down to the subsystem and reason.

---

## The API Design Philosophy

Bootstrap the syscall pipeline once (typical pattern):

```c
PVOID clean_ntdll = NULL;
snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &clean_ntdll);
snd_ntdll_set_clean(clean_ntdll);
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
snd_syscall_set_invoker(snd_syscall_direct_invoke_asm);
// or for indirect syscalls:
// snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
// snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);
// or for spoofed syscalls:
// snd_syscall_set_invoker(snd_syscall_spoofed_invoke_asm);
// snd_syscall_set_spoof_finder(snd_syscall_find_spoof_scan);
```

The invoker is decoupled from SSN resolution — switch between direct, indirect, and spoofed syscalls without modifying domain code. Indirect invocation jumps to a legitimate NTDLL gadget so the return address stays inside `ntdll.dll`; spoofed invocation additionally plants a genuine caller return address inside a dynamically discovered "Fat Frame", so call-stack unwinds stay coherent.

Swap execution profile with one assignment:

```c
ctx.mem_api = &snd_mem_win;   // diagnostic
ctx.mem_api = &snd_mem_nt;    // NT stubs via PEB + EAT
ctx.mem_api = &snd_mem_sys;   // direct syscalls (pipeline required)
```

Module resolution follows the same pattern (`snd_mod_win` vs `snd_mod_nt`). There is no syscall-backed module backend — imports use PEB walk + EAT even in full `_sys` profiles.

---

## Build Tiers

### Debug Tier — `SND_ENABLE_DEBUG=ON`

For local development. `snd_status_t` expands to include `file`, `line`, and a 128-byte `context` string buffer. `SND_ERR_CTX` and `SND_DEBUG_PRINT` emit state machine transitions, parsed PE field values, and syscall resolution outcomes. Use `SND_USE_PRINTF=ON` to route output to `stdout` instead of the debug console.

### Silent Tier — `SND_ENABLE_DEBUG=OFF`

The standard deployment configuration for operational binaries. Every diagnostic string, file reference, and line number compiles away completely. `snd_status_t` collapses to two integers. Nothing else. A `SND_CRTLESS=ON` build layers on `/NODEFAULTLIB`, no Windows SDK header, a PEB command-line frontend, and native backends only.

```cmake
set(SND_ENABLE_DEBUG   OFF   CACHE BOOL   "")
set(SND_BUILD_PAYLOADS OFF   CACHE BOOL   "")
set(SND_RANDOMIZE_SEED ON    CACHE BOOL   "")
set(SND_USE_DEFAULTS   ON    CACHE BOOL   "")
set(SND_HASH_ALGO    "DJB2"  CACHE STRING "")
add_subdirectory(vendor/SindriKit)
target_link_libraries(my_tool PRIVATE sindri::engine)
```

---

## Documentation

Full reference under [`docs/`](docs/README.md):

- **[API Reference](docs/api_reference.md)** — the complete public C API (functions, types, DI tables, status codes)
- **[Getting Started](docs/getting_started/README.md)** — build tiers, CMake integration, syscall bootstrap, first loader/injection workflow
- **[Architecture](docs/architecture/README.md)** — dependency injection, state machines, facility-encoded status system
- **[Primitives](docs/primitives/README.md)** — memory, modules, process, mapping, files, thread, syscalls, execution (FFI, Heaven's Gate)
- **[Loaders](docs/loaders/README.md)** — reflective PE and COFF/BOF loading
- **[Injection](docs/injection/README.md)** — classic, early-bird APC, and thread-hijack injection (shellcode, PE, COFF)
- **[Parsers](docs/parsers/README.md)** — PE, COFF, and env (PEB/NTDLL) parsing
- **[Common](docs/common/README.md)** — CRT-free helpers, buffers, hashing, status
- **[Examples & PoCs](docs/examples/README.md)** — the `unified` CLI (`load pe|coff`, `inject classic|apc|hijack`, `hg`)
- **[Tests](docs/tests/README.md)** — integration runners and the PE mutator

*Planned: an **Evasion** domain.*

---

## Disclaimer

**SindriKit is built for educational, research, and authorized Red Teaming purposes only.** For the full legal disclaimer and information regarding OpSec considerations, see the [Security Policy](SECURITY.md).

---

## License

[MIT](LICENSE)

---

<p align="center">
  <img src="assets/title.png" width="100%">
</p>
