# PoC: loader_noCRT_nowinapi

**Location:** `pocs/loader_noCRT_nowinapi/`

Minimal reflective loader with the Microsoft CRT stripped (`/NODEFAULTLIB`). Demonstrates SindriKit's CRT manifest fallbacks and the smallest viable integration surface for implant-style binaries.

## What it demonstrates

- Release build with `/NODEFAULTLIB` and `/ENTRY:main`
- PEB hash walk for `ntdll` (`snd_peb_get_module_base_hash`) — no disk read
- Syscall scan strategy (single resolver, no sort fallback)
- Mixed profile: `snd_mem_win` + `snd_mod_nt`
- Hardcoded payload path (edit source before building)

## Walkthrough

No CLI — edit `file_path` in `main.c` before building.

### 1. Bootstrap `ntdll` from PEB

```c
PVOID ntdll = NULL;
status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
snd_syscall_set_ntdll(ntdll);
snd_syscall_strategy_set(snd_syscall_resolve_ssn_scan);
```

### 2. Configure loader context

```c
const char *file_path = "payload.exe";

snd_ldr_pe_ctx_t ctx = {0};
ctx.mem_api = &snd_mem_win;
ctx.mod_api = &snd_mod_nt;
```

### 3. Load, prepare, execute

```c
status = snd_disk_buffer_load(file_path, &file_buf);
ctx.raw_source = &file_buf;

status = snd_ldr_pe_prepare_image(&ctx);
status = snd_ldr_pe_execute_image(&ctx);
```

No DLL export FFI (`-e`/`-a`).

### 4. Cleanup

```c
snd_ldr_pe_detach_image(&ctx);       // no-op unless stage == EXECUTED
snd_ldr_pe_free_mapped_image(&ctx);
snd_buffer_free(&file_buf);
```

## Building

CRT-less mode **replaces** the full PoC set with this target. Debug must be off:

```bash
cmake -B build -DSND_CRTLESS=ON -DSND_ENABLE_DEBUG=OFF -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
```

When `SND_CRTLESS=ON`, the engine compiles `src/common/crt_manifest.c` for intrinsic `memcpy`/`memset` fallbacks. PoC CMake adds:

```cmake
target_link_options(loader_noCRT_nowinapi PRIVATE /NODEFAULTLIB /ENTRY:main)
```

> [!WARNING]
> `SND_BUILD_TESTS=ON` forces `SND_CRTLESS=OFF`. Use a clean build directory when switching modes.

Output: `build/pocs/loader_noCRT_nowinapi/Release/loader_noCRT_nowinapi.exe`

## OpSec impact

- No implicit CRT telemetry or default library imports
- PEB-only `ntdll` resolution (no KnownDlls, no disk read)
- Memory still uses Win32 `VirtualAlloc` via `snd_mem_win` — swap to `snd_mem_sys` for full stealth

## See also

- [Building: CRT-less tier](../getting_started/building.md)
- [Common infrastructure](../common/infrastructure.md)
- [loader_nowinapi.md](loader_nowinapi.md) — full CLI loader with NT backends
