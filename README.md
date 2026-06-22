<p align="center">
  <img src="assets/banner.png" width="100%">
</p>

<h1 align="center">SindriKit</h1>

<p align="center">
  <strong>The infrastructure offensive development never had.</strong><br>
  <em>A foundational C library for building operationally credible offensive capabilities.</em>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/language-C-blue?style=flat-square"/>
  <img src="https://img.shields.io/badge/arch-x86%20%7C%20x64-lightgrey?style=flat-square"/>
  <img src="https://img.shields.io/badge/build-CMake-orange?style=flat-square"/>
  <img src="https://img.shields.io/badge/license-MIT-green?style=flat-square"/>
</p>

---

## What SindriKit Actually Is

**SindriKit is not an implementation. It is the architecture underneath implementations.**

Most offensive libraries give you parts. A syscall wrapper. A reflective loader. A process injector. Useful individually, incompatible by default, and you inherit the task of rewriting them from scratch every time an EDR changes its mind.

The real problem is that offensive tools couple two things that should never be coupled: the logic of a technique and the mechanics of its execution. Your loader doesn't just load a PE image. It loads a PE image *using VirtualAlloc, LoadLibrary... or whatever Win32 calls you hardcoded months ago*. When that gets flagged, you rewrite. The target changes. You rewrite again. You are not fighting the problem. You are fighting your own architecture.

SindriKit applies a single fix to that problem at the design level: **any offensive technique is expressed as a context structure that consumes injected API tables**. The execution mechanics (e.g. Win32, direct syscall, driver-backed, anything an operator defines) are resolved at runtime through those tables, completely invisible to the technique's logic.

The consequence is immediate: swapping your entire execution strategy from Win32-visible to raw direct-syscall becomes one line of code without having to rewrite your entire toolchain. The technique never knew the difference.

---

## What You Get

| | What it means |
|---|---|
| **One mental model** | Every domain ships with the same context + API table contract, standardized across each and every technique. |
| **Zero rewrite cycle** | Swap functions pointers to shift your entire execution profile. The technique never changes. |
| **Zero integration friction** | One `add_subdirectory` and `target_link_libraries` to inherit the full engine. |
| **Zero production overhead** | Silent tier compiles away every diagnostic string, file reference, and line number. |

---

## 60-Second Drop-In

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyTool C ASM_MASM)

set(SND_BUILD_PAYLOADS  OFF    CACHE BOOL   "")
set(SND_ENABLE_DEBUG    OFF    CACHE BOOL   "")
set(SND_HASH_ALGO      "DJB2"  CACHE STRING "")
set(SND_RANDOMIZE_SEED  ON     CACHE BOOL   "")

add_subdirectory(libs/SindriKit)

add_executable(my_tool src/main.c)
target_link_libraries(my_tool PRIVATE sindri::engine)
```

```sh
cmake -B build && cmake --build build --config Release
```

Just two lines for your tool to inherit all of SindriKit's capabilities: PE parsing, syscall resolution, reflective loading...

---

## The Engine

### API Abstraction Layer

```
        ┌────────────────────────────────────────────────────────────────────────────┐
        │                          ANY OFFENSIVE INTENT                              │
        │    Loader · Injector · Spoofer · Patcher · Bypasser · Harvester · ...      │
        ├────────────────────────────────────────────────────────────────────────────┤
        │                     SINDRIKIT API ABSTRACTION LAYER                        │
        │      snd_memory_api_t  ->  alloc · free · protect                          │
        │      snd_module_api_t  ->  load_library · get_proc_address · ...           │
        │      [ future tables ] ->  thread · process · object · ...                 │
        ├──────────────────┬──────────────────────┬──────────────────────────────────┤
        │   Win32 Profile  │    Native Profile    │    Bring Your Own Mechanic       │
        │  VirtualAlloc    │  NtAllocateVirtual   │  Driver · ROP · Exotic           │
        │  LoadLibraryA    │  PEB Walk + EAT      │  Operator-defined functions      │
        └──────────────────┴──────────────────────┴──────────────────────────────────┘
```

In practice, this means every domain follows the same contract:

```c
// Reflective Loader
snd_loader_ctx_t loader = {0};
loader.raw_source = &payload;
loader.mem_api    = &snd_mem_native;
loader.mod_api    = &snd_mod_native;
snd_prepare_reflective_image(&loader);
snd_execute_reflective_image(&loader);

// Remote Injector (planned)
snd_injector_ctx_t injector = {0};
injector.mem_api    = &snd_mem_native;
injector.target_pid = 1337;
snd_execute_injection(&injector);

// ETW Patcher, Stack Spoofer, Credential Harvester... Same contract, always.
```

### Cascading Syscall Pipeline

No single extraction technique works everywhere. The hook architecture inside an EDR is not what you'll find in another. Hell's Gate for example works until it doesn't.

SindriKit treats syscall resolution as an injectable mechanic. Stack strategies in priority order. The engine falls through until one succeeds:

```c
snd_set_syscall_strategy(snd_hell_extract_syscall);
snd_add_syscall_strategy(snd_halo_extract_syscall);
snd_add_syscall_strategy(snd_veles_extract_syscall);
snd_add_syscall_strategy(any_other_technique_you_want);
```

### Compile-Time Algorithm Agility

Every API name and module string is stripped from the final binary at compile time. Swap hashing algorithms across the entire codebase with a single CMake variable:

```cmake
set(SND_HASH_ALGO "FNV1A")  # or DJB2 recomputes everything automatically
set(SND_RANDOMIZE_SEED ON)  # generates a fresh 32-bit seed on next configure
```

Each hash is embedded alongside a randomly generated seed (if `SND_RANDOMIZE_SEED=ON`). Static footprint shifts completely between compilations without touching a line of C.

### Architecture-Aware Dynamic FFI

A custom MASM assembly bridge for arbitrary runtime function invocation. x64 builds follow the Microsoft x64 calling convention precisely (shadow space, register argument placement, stack alignment). x86 builds push arguments in reverse order with support for both `cdecl` and `stdcall` targets.

### Bounds-Checked PE Parser

A unified PE32/PE32+ parser with a critical `is_mapped` flag that correctly handles both raw on-disk images and memory-mapped views. Every data directory access is validated against tracked buffer bounds before dereferencing. Export resolution supports forwarder chains up to depth 4 with hash-based lookup.

Tested against inputs that actually break naive parsers:
- 40+ core test combinations targeting edge-case EXEs, DLLs, bad arguments, missing exports, and TLS callbacks across x86 and x64.
- 100+ dynamic PE mutations generated by the `pe_mutator` module: zeroed section names, integer overflows, invalid `e_lfanew` bounds, mangled imports.
- Full Corkami corpus: cleanly loads valid samples, cleanly rejects malformed ones With no crashes across 99% of the samples.

### State-Tracked Domain Contexts

Every offensive operation is managed through a discrete context structure with an explicit stage enumeration. Operations can be paused between stages for sleep obfuscation or staged deployment, resumed cleanly, and inspected for the exact failure point down to the subsystem and reason.

---

## The API Design Philosophy

> **A context structure owns the operation's state. Injected API tables own the execution mechanics. The two are completely separate.**

Bootstrap the execution subsystem once:

```c
PVOID clean_ntdll = NULL;
snd_map_knowndll(&snd_knowndlls_win, L"ntdll.dll", &clean_ntdll);
snd_set_ntdll(clean_ntdll);

snd_set_syscall_strategy(snd_hell_extract_syscall);
snd_add_syscall_strategy(snd_halo_extract_syscall);
```

One line to change your entire execution strategy:

```c
// Win32 profile
const snd_memory_api_t *mem_api = &snd_mem_win;

// Native profile. Everything above this line stays identical
const snd_memory_api_t *mem_api = &snd_mem_native;
```

`snd_module_api_t` follows the same contract. `mod_api->get_proc_address` resolves to `GetProcAddress` under `snd_mod_win` and to manual EAT parsing over PEB-walked module bases under `snd_mod_native`. The operator's algorithm never changes regardless of which profile is active.

---

## Build Tiers

### Debug Tier — `SND_ENABLE_DEBUG=ON`

For local development. `snd_status_t` expands to include `file`, `line`, and a 128-byte `context` string buffer. `SND_ERR_CTX` and `SND_DEBUG_PRINT` emit state machine transitions, parsed PE field values, and syscall resolution outcomes. Use `SND_USE_PRINTF=ON` to route output to `stdout` instead of the debug console.

### Silent Tier — `SND_ENABLE_DEBUG=OFF`

The only acceptable configuration for any binary destined for a target environment. Every diagnostic string, file reference, and line number compiles away completely. `snd_status_t` collapses to two integers. Nothing else.

```cmake
set(SND_ENABLE_DEBUG   OFF   CACHE BOOL   "")
set(SND_BUILD_PAYLOADS OFF   CACHE BOOL   "")
set(SND_RANDOMIZE_SEED ON    CACHE BOOL   "")
set(SND_HASH_ALGO    "DJB2"  CACHE STRING "")
add_subdirectory(vendor/SindriKit)
target_link_libraries(my_tool PRIVATE sindri::engine)
```

---

## Documentation

- **[Getting Started](docs/getting_started/)** — Installation, CMake integration, and bootstrapping the engine.
- **[Core Architecture](docs/architecture/)** — The design philosophy, state machines, dependency injection.
- **[Domain: Primitives](docs/domains/primitives/)** — Direct syscall pipelines (Hell's Gate / Halo's Gate etc).
- **[Domain: Loaders](docs/domains/loaders/)** — The staged reflective PE loading pipeline and KnownDlls bootstrapping.
- **[Parsers](docs/parsers/)** — Bounds-checked PE32/PE32+ parsing: exports, imports, relocations.
- **[Infrastructure](docs/common/)** — Common API definitions, configuration, and hash manifests.
- **[Tooling & Tests](docs/tests/)** — Test runner, PE mutator, and Python scripts.
- **[Examples & PoCs](docs/examples/)** — Step-by-step walkthroughs of `loader_winapi` and `loader_nowinapi`.

*Planned: **[Injection](docs/domains/injection/)** and **[Evasion](docs/domains/evasion/)** domains.*

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
