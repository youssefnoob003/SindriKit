# PoC: loader_winapi

**Location:** `pocs/loader_winapi/`

Diagnostic baseline for the reflective PE loader. Every memory and module operation uses standard Win32 APIs (`VirtualAlloc`, `LoadLibraryA`, etc.). Intended for pipeline validation and debugging — not operational deployment.

## What it demonstrates

- Full reflective chain: `snd_ldr_pe_prepare_image` → `snd_ldr_pe_execute_image`
- Auto-detected DLL vs EXE payloads
- Post-load DLL export invocation via `snd_ldr_pe_get_proc_address` + `snd_ffi_execute`
- Win32 DI profile: `snd_mem_win` + `snd_mod_win`

## Command-line usage

```text
loader_winapi -f <payload_path> [-e <export_name>] [-a <arg>]...

  -f   Path to PE payload (DLL or EXE)
  -e   Export name (required for DLL payloads after DllMain)
  -a   Argument to pass to export (decimal or 0x hex). Repeat up to 32 times.
       Non-numeric values are passed as string pointers.
```

**EXE:** entry point runs during `snd_ldr_pe_execute_image`; `-e`/`-a` are ignored.

**DLL:** `DllMain(DLL_PROCESS_ATTACH)` runs first, then `-e` export is resolved and called through the FFI bridge with `-a` arguments.

### Examples

```bash
# Run a reflective EXE payload locally
loader_winapi.exe -f payload.exe

# Load DLL, run DllMain, then call an export with two numeric args
loader_winapi.exe -f payload.dll -e Run -a 0x1 -a 0x2

# Pass a string pointer argument
loader_winapi.exe -f payload.dll -e PrintMsg -a "hello"
```

## Walkthrough

### 1. Load payload from disk

```c
snd_buffer_t     file_buf = {0};
snd_ldr_pe_ctx_t ctx      = {0};

status = snd_disk_buffer_load(file_path, &file_buf);
ctx.raw_source = &file_buf;
```

### 2. Inject Win32 primitives

No syscall bootstrap required.

```c
ctx.mem_api = &snd_mem_win;
ctx.mod_api = &snd_mod_win;
```

### 3. Prepare and execute

```c
status = snd_ldr_pe_prepare_image(&ctx);   // parse → map → reloc → imports → protect
status = snd_ldr_pe_execute_image(&ctx);  // TLS + DllMain or EXE entry
```

### 4. Optional DLL export call

```c
FARPROC fn = NULL;
snd_ldr_pe_get_proc_address(&ctx, export_name, &fn);
UINT_PTR ret = snd_ffi_execute((PVOID)(UINT_PTR)fn, call_argc, call_args);
```

### 5. Cleanup

```c
snd_ldr_pe_detach_image(&ctx);
snd_ldr_pe_free_mapped_image(&ctx);
snd_buffer_free(&file_buf);
```

## Building

Included in the default PoC build (`SND_CRTLESS=OFF`):

```bash
cmake -B build -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
```

## OpSec impact

Maximum telemetry: every allocation, protection change, and import resolution is visible to userland EDR hooks on `kernel32.dll` and `ntdll.dll`.

## See also

- [Loaders techniques](../domains/loaders/techniques.md)
- [loader_nowinapi.md](loader_nowinapi.md) — NT API profile
