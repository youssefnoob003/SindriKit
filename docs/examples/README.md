# Examples & Proofs of Concept

`pocs/` ships a single command-line executable, `unified`. It is intentionally thin: each command composes the same public loader, injection, primitive, and syscall APIs an implant integration would use.

| | |
|---|---|
| Source | `pocs/src/` |
| Target | `unified` (enabled by `SND_BUILD_PAYLOADS=ON`) |
| Entrypoint | `main` (SDK build) · `snd_crtless_poc_entry` (CRT-less build) |

## Commands

| Command | Modes | Default backend | Syscall invoke | Page |
|---|---|---|---|---|
| `unified load pe` | — | `--win`¹ | indirect | [load_pe.md](load_pe.md) |
| `unified load coff` | — | `--win`¹ | indirect | [load_coff.md](load_coff.md) |
| `unified inject classic` | `shell`, `pe`, `coff` | `--nt` | indirect | [inject_classic.md](inject_classic.md) |
| `unified inject apc` | `shell`, `pe`, `coff` | `--sys` | direct | [inject_apc.md](inject_apc.md) |
| `unified hg` | — | — | — | [heavens_gate.md](heavens_gate.md) |

¹ CRT-less builds default to `--nt` and reject `--win`.

Every command accepts `-h`/`--help`. Grammar, backend tables, syscall strategy flags, and argument-passing rules are documented once in [cli.md](cli.md).

## Build

Normal build (Microsoft CRT, Windows SDK available), both architectures:

```bat
build.bat pocs
:: → build32/pocs/Release/unified.exe (x86)
:: → build64/pocs/Release/unified.exe (x64)
```

CRT-less build (`/NODEFAULTLIB`, PEB frontend, silent output):

```bat
build.bat pocs crtless
```

Equivalent raw CMake (single architecture, `build/` tree):

```bash
cmake -B build -A x64 -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
# → build/pocs/Release/unified.exe
```

See [building.md](../getting_started/building.md) for every keyword and cache variable.

## Shared conventions

- **Backends.** `--win`, `--nt`, and `--sys` swap the injected execution mechanics without changing command logic. `--nt`/`--sys` use the native file APIs, so `-f` (and `-t` for APC) must be fully qualified absolute paths; `--win` accepts relative paths.
- **Syscall bootstrap.** `--sys` maps a clean `ntdll` from KnownDlls, registers it with `snd_ntdll_set_clean`, and configures the pipeline via `apply_syscall_style` before the command body runs.
- **Exit codes.** Commands return `SND_SUCCESS` (0) or the failing `snd_status_t.code`. The CRT-less frontend passes that code to `RtlExitUserProcess`.

## Page map

- [cli.md](cli.md) — command grammar, options, backend/strategy reference, source layout
- [load_pe.md](load_pe.md) — `unified load pe`: reflective PE execution across Win32/NT/syscall profiles
- [load_coff.md](load_coff.md) — `unified load coff`: in-memory COFF/BOF execution
- [inject_classic.md](inject_classic.md) — `unified inject classic`: shellcode, PE, and COFF remote injection
- [inject_apc.md](inject_apc.md) — `unified inject apc`: early-bird APC injection into a spawned target
- [heavens_gate.md](heavens_gate.md) — `unified hg`: WoW64 → native x64 execution
- [crtless.md](crtless.md) — CRT-less target, PEB frontend, and silent logging

## Related documentation

- [Getting started: basic usage](../getting_started/basic_usage.md)
- [Loaders domain](../loaders/README.md)
- [Injection domain](../injection/README.md)
