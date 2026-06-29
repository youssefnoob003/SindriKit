# Red Team Integration

SindriKit is designed as an **embeddable static engine** (`sindri::engine`) inside C2 implants, custom loaders, and staging pipelines — not only as standalone PoCs. Domain logic (reflective load, classic inject) stays in the library; the implant supplies intent, configuration, and chosen OpSec profile.

---

## CMake embedding

```cmake
cmake_minimum_required(VERSION 3.16)
project(Implant C CXX ASM_MASM)

set(SND_BUILD_PAYLOADS OFF CACHE BOOL "")
set(SND_ENABLE_DEBUG OFF CACHE BOOL "")
set(SND_CRTLESS ON CACHE BOOL "")          # optional: /NODEFAULTLIB profile
set(SND_HASH_ALGO "DJB2" CACHE STRING "")

add_subdirectory(vendor/SindriKit)

add_executable(implant src/main.c)
target_link_libraries(implant PRIVATE sindri::engine)
```

| Target | Alias | Contents |
|---|---|---|
| `sindri_engine` | `sindri::engine` | All primitives, parsers, loaders, injection, MASM stubs |

The build runs `scripts/generate_hashes.py` at configure time and compiles architecture-specific ASM (`ffi`, syscalls, Heaven's Gate) automatically.

Full variable reference: [Building SindriKit](../getting_started/building.md).

---

## Minimal implant init pattern

Typical startup sequence for a syscall-hardened implant:

```c
#include <sindri.h>

static snd_status_t bootstrap_engine(void) {
    snd_status_t status;
    PVOID ntdll = NULL;

    status = snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);
    if (SND_FAILED(status))
        return status;

    snd_syscall_set_ntdll(ntdll);
    snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
    snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
    
    snd_syscall_set_invoker(snd_syscall_direct_invoke_asm);
    // or for indirect syscalls:
    // snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
    // snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);

    return SND_OK;
}

static snd_status_t run_payload(snd_buffer_t *pe) {
    snd_ldr_pe_ctx_t ctx = {0};
    ctx.mem_api    = &snd_mem_sys;
    ctx.mod_api    = &snd_mod_nt;
    ctx.raw_source = pe;

    snd_status_t status = snd_ldr_pe_prepare_image(&ctx);
    if (SND_FAILED(status))
        return status;

    return snd_ldr_pe_execute_image(&ctx);
}
```

Remote injection swaps in `snd_inj_ctx_t` + `snd_inj_classic_pe` — see [Basic usage](../getting_started/basic_usage.md).

---

## Include strategy

| Include | When to use |
|---|---|
| `sindri.h` | Full toolkit — loaders, injection, parsers, primitives |
| `sindri/primitives.h` | Memory/process/syscall only, no loader |
| `sindri/loaders.h` | Reflective PE without injection |
| `sindri/injection.h` | Injection without local loader (shellcode path) |
| `sindri/common.h` | Status/buffer/hash only |

Granular includes reduce compile time and make dependency boundaries explicit in large codebases.

---

## Algorithm agility (hash rotation)

API and module names resolve through compile-time hashes from `config/hashes.ini`. CMake controls the algorithm and seed:

```cmake
set(SND_HASH_ALGO "FNV1A" CACHE STRING "")
set(SND_RANDOMIZE_SEED ON CACHE BOOL "")
```

Configure regenerates hash constants — **no C source edits** when rotating algorithms or seeds. Manifest format: [Hashes manifest](../config/hashes_manifest.md).

Runtime hashing (`snd_hash`, `snd_hash_lower`, `snd_hash_wide_lower`) uses the same algorithm selected at configure time.

---

## Build profiles for operations

| Goal | Settings |
|---|---|
| Local dev / triage | `SND_ENABLE_DEBUG=ON`, optional `SND_USE_PRINTF=ON` |
| Production implant | `SND_ENABLE_DEBUG=OFF`, `SND_CRTLESS=ON` if avoiding CRT |
| PoC verification | `SND_BUILD_PAYLOADS=ON`, DEBUG optional |

SILENT + CRT-less is the standard production combination: no `.rdata` error strings, no `stdio` linkage from status macros.

---

## Bring your own mechanics (BYOM)

Existing implant allocators, module walkers, or process openers can replace stock backends by filling `snd_memory_api_t`, `snd_module_api_t`, or `snd_process_api_t` and injecting pointers into SindriKit contexts.

SindriKit's state machines and PE logic remain unchanged — only the OS contact layer swaps.

Example: [Dependency injection — custom backends](dependency_injection.md#custom-backends-byom).

---

## PoCs as integration references

| PoC | Integration lesson |
|---|---|
| `loader_winapi` | Simplest DI — Win32 tables, full chain |
| `loader_nowinapi` | NT tables + syscall bootstrap from disk `ntdll` |
| `loader_noCRT_nowinapi` | `/NODEFAULTLIB` + minimal surface |
| `inject_pe` | Dual context — loader bake + remote syscall inject |
| `inject_shell` | Injection-only, Win32 process API |
| `heavens_gate` | x86 WoW64 execution primitive (orthogonal to syscalls) |

Walkthroughs: [Examples](../examples/README.md).

---

## Testing before embed

Enable the internal test suite when integrating into a new parent build (requires CRT):

```cmake
set(SND_BUILD_TESTS ON CACHE BOOL "")
```

Documentation: [Test runner](../tests/test_runner.md).

For quick validation without parent CMake, build PoCs standalone:

```bash
cmake -B build -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
```

---

## Domain boundaries in implants

Do not link implant-specific evasion code into SindriKit source trees. Instead:

1. Implement evasion as custom DI backends or pre-bootstrap hooks in **your** `main.c`.
2. Keep SindriKit vendor copy clean for upstream merges.
3. Respect domain independence — loaders must not call injection internals directly; use documented chain APIs.

Architecture overview: [README](README.md) · [Domains](../domains/README.md).

---

## Related documentation

- [Dependency injection](dependency_injection.md)
- [Status system](status_system.md) — SILENT tier requirements
- [Architecture README](README.md)
