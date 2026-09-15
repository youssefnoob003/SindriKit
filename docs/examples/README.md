# Examples & Proofs of Concept

The repository ships a single `unified` executable under `pocs/`. It demonstrates the same public loader, injection, parser, primitive, and syscall APIs used by implant integrations.

The normal executable is built with `SND_USE_WINDOWS_SDK=1` because it exposes the Win32 backend and includes `<windows.h>`. CRT-less builds use the same command implementation with a Sindri-only frontend and native backend selection; the CLI remains silent because it has no CRT console implementation.

## Command reference

- [unified.md](unified.md) — build instructions, command syntax, backend selection, and source organization

## Domain walkthroughs

The focused pages remain useful as technique documentation:

- [loader_winapi.md](loader_winapi.md) — Win32 backend profile for `unified load pe`
- [loader_nowinapi.md](loader_nowinapi.md) — native NT backend profile for `unified load pe`
- [loader_noCRT_nowinapi.md](loader_noCRT_nowinapi.md) — CRT-less integration constraints
- [inject_shell.md](inject_shell.md) — classic shellcode injection
- [inject_pe.md](inject_pe.md) — classic PE injection
- [heavens_gate.md](heavens_gate.md) — WoW64 to native x64 execution

These pages describe profiles and API composition; their executable examples now live in `pocs/src/` and are invoked through the unified command.

## Building all PoCs

```bash
cmake -B build -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
```

Binaries land under `build/pocs/Release/unified.exe` (MSVC multi-config) or `build/pocs/unified` (single-config generators).

CRT-less unified PoC:

```bash
cmake -B build -DSND_CRTLESS=ON -DSND_ENABLE_DEBUG=OFF -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
```

The CRT-less target is still named `unified`. It reads the process command line from the PEB, converts it to the shared command argument model, and runs the same loader and injection commands using native Sindri primitives. Heaven's Gate remains available only to x86 builds.

## Related documentation

- [Getting started: basic usage](../getting_started/basic_usage.md)
- [Loaders domain](../loaders/README.md)
- [Injection domain](../injection/README.md)
