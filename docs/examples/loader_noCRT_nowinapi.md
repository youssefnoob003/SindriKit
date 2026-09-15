# Example: CRT-less engine integration

**Reference implementation:** `src/common/crt_manifest.c` and the CRT-safe common headers
**Build mode:** `SND_CRTLESS=ON`

CRT-less reflective-loader profile with the Microsoft CRT stripped (`/NODEFAULTLIB`). Demonstrates SindriKit's CRT manifest fallbacks and the smallest viable integration surface for implant-style binaries.

## What it demonstrates

- Release build with `/NODEFAULTLIB` and the CRT-less unified entrypoint
- PEB hash walk for `ntdll` (`snd_peb_get_module_base_hash`) — no disk read
- Syscall scan strategy (single resolver, no sort fallback)
- Native profile: `snd_file_nt` + `snd_mem_nt` + `snd_mod_nt`
- Native file loading through `snd_file_nt`; the example uses a payload path supplied by the command line or integration wrapper

## Walkthrough

The CRT-less `unified` target is command-line compatible with the normal target: its direct entrypoint reads and tokenizes the process command line, then calls the same shared dispatcher. It intentionally keeps output silent because the CRT-less print adapter is a no-op. For a smaller integration, build the engine with `SND_CRTLESS=ON` and embed it in a `/NODEFAULTLIB` executable whose entry point supplies the payload path and backend configuration.

### 1. Bootstrap `ntdll` from PEB

```c
PVOID ntdll = NULL;
status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
snd_ntdll_set_clean(ntdll);
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
```

### 2. Configure loader context

```c
const char *file_path = "payload.exe";

snd_ldr_pe_ctx_t ctx = {0};
ctx.mem_api = &snd_mem_nt;
ctx.mod_api = &snd_mod_nt;
```

### 3. Load, prepare, execute

```c
status = snd_file_nt.load(file_path, &file_buf);
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

CRT-less mode builds the engine and the shared `unified` command implementation with its CRT-less frontend. Debug and console output must be off:

```bash
cmake -B build -DSND_CRTLESS=ON -DSND_ENABLE_DEBUG=OFF -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
```

When `SND_CRTLESS=ON`, the engine compiles `src/common/crt_manifest.c` for intrinsic `memcpy`/`memset` fallbacks. The CRT-less `unified` target uses `pocs/src/frontend_crtless.c` plus the same shared command sources as the normal PoC, `/NODEFAULTLIB`, native file loading, and a direct entrypoint. It performs no console output and includes no Windows SDK header.

> [!WARNING]
> `SND_BUILD_TESTS=ON` forces `SND_CRTLESS=OFF`. Use a clean build directory when switching modes.

The output path is controlled by the consuming implant target.

## OpSec impact

- No implicit CRT telemetry or default library imports
- PEB-only `ntdll` resolution (no KnownDlls, no disk read)
- Memory, mapping, module, process, thread, and file operations use the native backend profile; syscall backends remain selectable in the normal unified command.

## See also

- [Building: CRT-less tier](../getting_started/building.md)
- [Common infrastructure](../common/infrastructure.md)
- [loader_nowinapi.md](loader_nowinapi.md) — full CLI loader with NT backends
