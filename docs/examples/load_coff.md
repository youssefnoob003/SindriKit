# `unified load coff` — In-Memory COFF/BOF Execution

**Implementation:** `pocs/src/cmd_load_coff.c`
**Syntax:** `unified load coff -f <payload.obj> [-e <entry>] [-a <arg>]... [--win|--nt|--sys]`

Loads an unlinked COFF object file, resolves its BOF-style symbols, applies relocations, and executes a named entry point in-process.

## What it demonstrates

- COFF parse → section allocation → symbol resolution → relocation → execute
- BOF entry-point selection (`-e`, default `go`)
- Packed argument marshaling: repeatable `-a` values are passed as a `UINT_PTR[]` buffer plus byte length
- The same backend swapping as the PE loader (`snd_mem_*` / `snd_mod_*`)

## Options

```text
  -f <path>            Path to the COFF object file (.obj). Required.
  -e <name>            Entry-point symbol to execute (default: "go").
  -a <arg>             Argument to pass to the BOF. Repeatable (max 32).
  --win                Win32 API backend (default outside CRT-less builds).
  --nt                 Native API backend (ntdll exports).
  --sys                Direct syscalls (KnownDlls clean ntdll + SSN).
  --invoke-direct      Syscall invoker: direct assembly.
  --invoke-indirect    Syscall invoker: indirect assembly (default).
  --invoke-spoofed     Syscall invoker: spoofed frame assembly.
  --resolve-scan       SSN resolver: in-memory scan (default).
  --resolve-sort       SSN resolver: export-table sort.
```

> [!NOTE]
> `load coff` treats `-a` like `load pe`: repeatable, each token numeric-by-value or string-by-pointer, marshaled as a packed buffer. The injection commands use a single raw string instead — see [cli.md](cli.md).

## Backend profiles

| Backend | `ctx.mem_api` | `ctx.mod_api` | File loader |
|---|---|---|---|
| `--win` | `snd_mem_win` | `snd_mod_win` | `snd_file_win` |
| `--nt` | `snd_mem_nt` | `snd_mod_nt` | `snd_file_nt` |
| `--sys` | `snd_mem_sys` | `snd_mod_nt` | `snd_file_sys` |

`--nt`/`--sys` require an absolute payload path.

## Walkthrough

### 1. Bind the backend and context

```c
unified_backend_t be = {0};
unified_backend_init(backend, &scfg, &be);  // API_SYS also bootstraps KnownDlls ntdll

snd_ldr_coff_ctx_t ctx = {0};
ctx.mem_api = be.mem_api;
ctx.mod_api = be.mod_api;
```

### 2. Load and execute

```c
snd_buffer_t file_buf = {0};
unified_file_load(&be, file_path, &file_buf);
ctx.raw_source = &file_buf;

snd_ldr_coff_load(&ctx);  // parse, allocate/copy sections, resolve symbols, apply relocations

char *bof_args    = call_argc > 0 ? (char *)call_args : NULL;  // packed UINT_PTR[]
int   bof_arg_len = call_argc * sizeof(UINT_PTR);

snd_ldr_coff_execute_image(&ctx, entry_name, bof_args, bof_arg_len);
```

### 3. Cleanup

```c
snd_ldr_coff_free_mapped_image(&ctx);
snd_buffer_free(&file_buf);
```

## Related injection path

`unified inject classic coff` and `unified inject apc coff` run the same COFF preparation locally but marshal the image into a remote process instead of executing it in-process. Their `-a` convention is a single raw string buffer. See [inject_classic.md](inject_classic.md) and [inject_apc.md](inject_apc.md).

## OpSec impact

- `--win`: standard `VirtualAlloc`/`LoadLibrary`-class telemetry.
- `--nt`: no Win32 memory APIs; in-process `ntdll` stubs still run.
- `--sys`: memory operations bypass userland stubs; external OS API symbols are resolved through the BOF symbol resolver (`MODULE$Function`).

## See also

- [COFF loader internals](../loaders/internals.md)
- [COFF parser](../parsers/coff/README.md)
- [cli.md](cli.md) — backend and argument reference
