# `unified load pe` — Reflective PE Execution

**Implementation:** `pocs/src/cmd_load_pe.c`
**Syntax:** `unified load pe -f <payload_path> [-e <export_name>] [-a <arg>]... [--win|--nt|--sys]`

Runs the full reflective PE pipeline locally. The same command covers the diagnostic Win32 profile, the evasive NT profile, and the syscall profile — only the injected primitive tables change.

## What it demonstrates

- Reflective chain: `snd_ldr_pe_prepare_image` → `snd_ldr_pe_execute_image`
- Auto-detected DLL vs EXE payloads
- Post-load DLL export invocation through `snd_ldr_pe_get_proc_address` + `snd_ffi_execute`
- Backend swapping via `ctx.mem_api` / `ctx.mod_api` without touching loader logic
- `--sys` bootstrap: KnownDlls-mapped clean `ntdll` + syscall pipeline configuration

## Options

```text
  -f <path>            Path to the PE payload (DLL or EXE). Required.
  -e <name>            Export to invoke. Required for DLL payloads.
  -a <arg>             Export argument. Repeatable (max 32).
  --win                Win32 API backend (default outside CRT-less builds).
  --nt                 Native API backend (ntdll exports).
  --sys                Direct syscalls (KnownDlls clean ntdll + SSN).
  --invoke-direct      Syscall invoker: direct assembly.
  --invoke-indirect    Syscall invoker: indirect assembly (default).
  --invoke-spoofed     Syscall invoker: spoofed frame assembly.
  --resolve-scan       SSN resolver: in-memory scan (default).
  --resolve-sort       SSN resolver: export-table sort.
```

Invoker/resolver flags are only effective with `--sys`. See [cli.md](cli.md) for exact flag semantics.

## Payload behavior

| Payload | Behavior |
|---|---|
| EXE | `snd_ldr_pe_execute_image` runs the entry point. `-e` and `-a` are ignored. |
| DLL | `DllMain(DLL_PROCESS_ATTACH)` (plus TLS callbacks) runs first; then the `-e` export is resolved and called through the FFI bridge with the `-a` arguments. If `-e` is missing the command fails after the image is prepared. |

`-a` tokens that parse fully as base-0 integers are passed by value; any other token is passed as a pointer to the raw string (`snd_ffi_execute` receives a `UINT_PTR[]`). See [cli.md](cli.md).

## Backend profiles

| Backend | `ctx.mem_api` | `ctx.mod_api` | File loader | Notes |
|---|---|---|---|---|
| `--win` | `snd_mem_win` | `snd_mod_win` | `snd_file_win` | Maximum telemetry; supports relative paths |
| `--nt` | `snd_mem_nt` | `snd_mod_nt` | `snd_file_nt` | Avoids Win32 memory/module APIs; absolute paths required |
| `--sys` | `snd_mem_sys` | `snd_mod_nt` | `snd_file_sys` | Direct syscalls for memory; imports still use PEB + EAT |

## Walkthrough

### 1. Bind the backend

```c
unified_backend_t be = {0};
unified_backend_init(backend, &scfg, &be);
// API_SYS: maps a clean KnownDlls ntdll, registers it, applies the syscall style,
//          and binds snd_file_sys / snd_mem_sys / snd_mod_nt
// API_NT:  binds snd_file_nt / snd_mem_nt / snd_mod_nt
// API_WIN: binds snd_file_win / snd_mem_win / snd_mod_win

snd_ldr_pe_ctx_t ctx = {0};
ctx.mem_api = be.mem_api;
ctx.mod_api = be.mod_api;
```

### 2. Load the payload from disk

`unified_file_load` dispatches through the bound file table.

```c
snd_buffer_t file_buf = {0};
unified_file_load(&be, file_path, &file_buf);
ctx.raw_source = &file_buf;
```

### 3. Prepare, execute, and call an export

```c
snd_ldr_pe_prepare_image(&ctx);   // parse → allocate/copy → reloc → imports → protect
snd_ldr_pe_execute_image(&ctx);   // TLS + DllMain / EXE entry point

FARPROC proc = NULL;
snd_ldr_pe_get_proc_address(&ctx, export_name, &proc);
UINT_PTR rv = snd_ffi_execute((PVOID)(UINT_PTR)proc, call_argc, call_args);
```

### 4. Cleanup

```c
snd_ldr_pe_detach_image(&ctx);       // DllMain(DETACH) + free when the image executed
snd_ldr_pe_free_mapped_image(&ctx);  // idempotent free for partially-prepared images
snd_buffer_free(&file_buf);
```

## OpSec impact

| Backend | Local telemetry |
|---|---|
| `--win` | Every allocation, protection change, and import resolution is visible to userland hooks on `kernel32.dll`/`ntdll.dll` |
| `--nt` | No `VirtualAlloc`/`LoadLibraryA` calls, but in-process `ntdll` stubs still run and hooks may fire |
| `--sys` | Memory operations bypass userland `ntdll` stubs; imports remain visible during PEB/EAT resolution; reading `ntdll` from KnownDlls avoids disk I/O |

## See also

- [PE loader internals](../loaders/internals.md)
- [load_coff.md](load_coff.md) — COFF/BOF execution through the same command family
- [inject_classic.md](inject_classic.md) — the same payloads injected into a remote process
- [cli.md](cli.md) — backend and syscall strategy reference
