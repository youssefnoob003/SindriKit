# Unified CLI Reference

`unified` is a single executable that dispatches to thin command implementations under `pocs/src/`. This page documents the grammar, options, backend mapping, syscall strategy selection, and argument semantics shared by every command.

## Grammar

```text
unified load pe      <options>
unified load coff    <options>
unified inject classic <mode> <options>
unified inject apc     <mode> <options>
unified inject hijack  <mode> <options>
unified hg                          # x86 builds only
```

Every command accepts `-h` / `--help`. Unknown commands, subcommands, or options print usage and return `SND_STATUS_INVALID_COMMAND_LINE_ARG`. A bare `unified` invocation returns `SND_STATUS_MISSING_COMMAND_LINE_ARGS`.

## Options by command

| Option | Commands | Meaning |
|---|---|---|
| `-f <path>` | all except `hg` | Payload path (PE, COFF, or raw shellcode depending on command/mode) |
| `-e <name>` | `load pe`, `load coff`, `inject */coff` | DLL export (`load pe`) or BOF entry symbol (COFF paths; default `go`) |
| `-a <arg>` | `load pe`, `load coff`, `inject classic/apc/hijack coff` | Arguments; encoding differs per command — see below |
| `-p <pid>` | `inject classic` | Target process ID, base 0 (decimal or `0x` hex) |
| `-t <path>` | `inject apc`, `inject hijack` | Executable image to spawn as the suspended target |
| `--win` / `--nt` / `--sys` | all except `hg` | Execution backend (see below) |
| `--invoke-direct` / `--invoke-indirect` / `--invoke-spoofed` | all except `hg` | Syscall invoker; only effective with `--sys` |
| `--resolve-scan` / `--resolve-sort` | all except `hg` | SSN resolver selection; only effective with `--sys` |
| `--sys-cache` | all except `hg` | Memoize resolved syscall entries after first success; only effective with `--sys` |

### `-a` argument encoding

| Command | Repeatable | Encoding |
|---|---|---|
| `load pe` | yes (max 32) | Each token that parses fully as a base-0 integer is passed by value; otherwise the raw argument string pointer is passed. Consumed by `snd_ffi_execute` as a `UINT_PTR[]` array. |
| `load coff` | yes (max 32) | Same parsing as `load pe`; marshaled to `snd_ldr_coff_execute_image` as a packed `UINT_PTR[]` buffer with a byte length. |
| `inject classic coff`, `inject apc coff`, `inject hijack coff` | no (last wins) | A single raw string buffer; length passed to the chain is `strlen + 1`. |
| `inject classic shell/pe`, `inject apc shell/pe`, `inject hijack shell/pe` | no | Parsed but unused. |

> [!NOTE]
> Because non-numeric `-a` values are passed as pointers into `argv`, they remain valid only for the lifetime of the process/command. The COFF injection modes pass a single string buffer instead, matching the classic BOF argument convention.

## Backends

`--win`, `--nt`, and `--sys` select which primitive tables the command injects. The command body itself is backend-agnostic.

| Backend | Files | Memory | Modules | Process | Thread |
|---|---|---|---|---|---|
| `--win` | `snd_file_win` | `snd_mem_win` | `snd_mod_win` | `snd_proc_win` | `snd_thread_win` |
| `--nt` | `snd_file_nt` | `snd_mem_nt` | `snd_mod_nt` | `snd_proc_nt` | `snd_thread_nt` |
| `--sys` | `snd_file_sys` | `snd_mem_sys` | `snd_mod_nt` | `snd_proc_sys` | `snd_thread_sys` |

- There is no syscall-backed module resolver: `--sys` still uses `snd_mod_nt` (PEB walk + EAT) for import resolution.
- `--win` is unavailable in CRT-less builds: backend initialization rejects it with `SND_STATUS_INVALID_COMMAND_LINE_ARG`, and the default becomes `--nt`.
- `--nt` and `--sys` load files through the NT file API, which requires fully qualified paths (e.g. `C:\path\payload.dll`). Relative paths fail with `STATUS_OBJECT_PATH_NOT_FOUND`. `--win` supports relative paths.

### `--sys` bootstrap

Every command binds its execution mechanics through one call:

```c
unified_backend_t be = {0};
unified_backend_init(backend, &scfg, &be);
// be.file_api / be.mem_api / be.mod_api / be.proc_api / be.thread_api
```

For `--sys`, `unified_backend_init` maps a clean `ntdll` from KnownDlls (no disk I/O), calls `snd_ntdll_set_clean`, and applies the syscall style before returning the bound tables.

`apply_syscall_style` (`pocs/src/backend.c`) maps the CLI flags onto the pipeline:

| Flags | Invoker | Resolver chain |
|---|---|---|
| *(none)* | indirect (unless command default differs) | scan → sort fallback |
| `--invoke-direct` | `snd_syscall_direct_invoke_asm` | unchanged |
| `--invoke-indirect` | `snd_syscall_indirect_invoke_asm` | unchanged |
| `--invoke-spoofed` | `snd_syscall_spoofed_invoke_asm` + spoof finder | unchanged |
| `--resolve-scan` only | unchanged | scan only |
| `--resolve-sort` only | unchanged | sort only |
| `--resolve-scan --resolve-sort` | unchanged | scan → sort fallback |
| `--sys-cache` | unchanged | unchanged; successful resolutions are memoized |

- The gadget finder (`snd_syscall_find_gadget_scan`) is always installed, regardless of invoker.
- Invoker flags override the command default; resolver flags never add both strategies unless both are given (or neither is).
- The cache is bypassed whenever `--invoke-spoofed` is set, since spoofed invocation rotates its gadget per call.
- If a flag appears more than once, the last occurrence wins.

## Defaults

| Command | Backend | Syscall invoker | Resolver chain |
|---|---|---|---|
| `load pe` | `--win`¹ | indirect | scan → sort |
| `load coff` | `--win`¹ | indirect | scan → sort |
| `inject classic` | `--nt` | indirect | scan → sort |
| `inject apc` | `--sys` | direct | scan → sort |
| `inject hijack` | `--nt` | indirect | scan → sort |

¹ `--nt` in CRT-less builds.

## Exit behavior

- Success returns `SND_SUCCESS` (0); failure returns the `snd_status_t.code` as the process exit code (debug builds additionally print the status context to `stderr`).
- `-h`/`--help` prints usage to `stderr` and exits successfully.
- CRT-less builds exit through `RtlExitUserProcess(status.code)` after `unified_main` returns.

## Source layout

| File | Responsibility |
|---|---|
| `pocs/src/main.c` | Global dispatch (`load`, `inject`, `hg`) and the CRT `main`; `hg` is compiled only on x86 |
| `pocs/src/frontend_crtless.c` | PEB command-line frontend and `snd_crtless_poc_entry` (CRT-less builds only) |
| `pocs/src/cli.c` | Shared option parsing (`require_arg`, backend/style flags, `-a` encodings, backend names) |
| `pocs/src/runtime.c` | CRT / CRT-less string and numeric adapters |
| `pocs/src/print.c` | Console logging (`[*]`, `[+]`, `[-]`, usage) or silent no-ops |
| `pocs/src/backend.c` | `unified_backend_init` (table binding + `--sys` bootstrap) and `apply_syscall_style` |
| `pocs/src/cmd_load_pe.c` | `unified load pe` |
| `pocs/src/cmd_load_coff.c` | `unified load coff` |
| `pocs/src/cmd_inject_classic.c` | `unified inject classic {shell,pe,coff}` |
| `pocs/src/cmd_inject_apc.c` | `unified inject apc {shell,pe,coff}` |
| `pocs/src/cmd_inject_hijack.c` | `unified inject hijack {shell,pe,coff}` |
| `pocs/src/cmd_hg.c` | `unified hg` |

The command layer never reimplements library logic; it only parses flags and composes public APIs.

## See also

- [Examples index](README.md)
- [Syscall pipeline](../primitives/syscalls/pipeline.md)
- [Building](../getting_started/building.md)
