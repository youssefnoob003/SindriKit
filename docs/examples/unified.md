# Unified PoC CLI

The repository ships one command-line PoC target, `unified`.

**Source:** `pocs/src/`  
**Target:** `unified`

Build it with:

```bash
build.bat pocs
```

On an MSVC multi-config build, the executable is normally written to `build/pocs/Release/unified.exe`.

## Commands

```text
unified load pe <options>
unified load coff <options>
unified inject classic <mode> <options>
unified inject apc <mode> <options>
unified hg <options>                 # x86/WoW64 only
```

Use `--help` after any command for the complete option list.

> [!WARNING]
> **Path Resolution:** When using the Native (`--nt`) or Syscall (`--sys`) backends, all file paths provided via `-f` and `-t` MUST be fully qualified absolute paths (e.g. `C:\path\to\payload.dll` or `Z:\SindriKit\build64\...\payload.dll`). Relative paths are not supported by the NT file APIs without providing a root directory handle, and will fail with `0xC000003A` (`STATUS_OBJECT_PATH_NOT_FOUND`). The Win32 backend (`--win`) fully supports relative paths.

### Load a PE

```text
unified load pe -f payload.dll -e ExportName --win
unified load pe -f payload.dll -e ExportName --nt
unified load pe -f payload.dll -e ExportName --sys --invoke-indirect
unified load pe -f payload.exe --win
```

`-a` may be repeated to pass up to 32 numeric or string-pointer arguments to a DLL export. EXE payloads execute their entry point and ignore `-e` and `-a`.

### Load a COFF object

```text
unified load coff -f payload.obj --sys
```

### Classic injection

```text
unified inject classic shell -f shellcode.bin -p 1234 --win
unified inject classic pe -f payload.dll -p 1234 --sys --invoke-indirect
unified inject classic coff -f payload.obj -p 1234 --nt
```

PE and COFF modes perform local preparation before the remote write and execution stages. `-e` and `-a` select the COFF entry point and argument string where supported.

### Early-bird APC injection

```text
unified inject apc pe -f payload.dll -p 1234 --sys
unified inject apc shell -f shellcode.bin -p 1234 --nt
```

The APC command creates or acquires the suspended target required by the selected technique and queues execution before resuming it.

### Heaven's Gate

Build an x86 target and run:

```text
unified hg <options>
```

This command is available only in 32-bit builds running under WoW64.

## Backend selection

`--win`, `--nt`, and `--sys` select the injected implementation tables:

| Option | Meaning |
|---|---|
| `--win` | Call the Win32 backend through the Windows SDK |
| `--nt` | Resolve and call native NT exports |
| `--sys` | Resolve SSNs and invoke configured syscall stubs |

For `--sys`, the CLI maps a clean `ntdll` image and configures the syscall pipeline. `--invoke-direct`, `--invoke-indirect`, `--invoke-spoofed`, `--resolve-scan`, and `--resolve-sort` select the syscall strategy.

## Source organization

The CLI is intentionally thin:

| File | Responsibility |
|---|---|
| `pocs/src/main.c` | Command dispatch and normal CRT entrypoint |
| `pocs/src/frontend_crtless.c` | PEB command-line entrypoint and direct process entry |
| `pocs/src/cli.c` | Shared option parsing |
| `pocs/src/runtime.c` | CRT or CRT-less string and numeric adapters |
| `pocs/src/print.c` | Console logging or CRT-less silent logging |
| `pocs/src/cmd_load_pe.c` | Reflective PE loading |
| `pocs/src/cmd_load_coff.c` | COFF loading |
| `pocs/src/cmd_inject_classic.c` | Classic injection |
| `pocs/src/cmd_inject_apc.c` | Early-bird APC injection |
| `pocs/src/cmd_hg.c` | Heaven's Gate |
| `pocs/src/syscall_cfg.c` | Syscall configuration |

The command layer does not replace the public library APIs; it demonstrates how to compose them.
