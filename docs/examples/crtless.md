# CRT-less `unified` Build

**Frontend:** `pocs/src/frontend_crtless.c`
**Build flag:** `SND_CRTLESS=ON`

The CRT-less build compiles the same command implementations against a Sindri-only frontend: no Microsoft CRT, no Windows SDK headers, no console output. It is the closest PoC profile to an implant-shaped binary.

## What it demonstrates

- `/NODEFAULTLIB` linking with the Sindri CRT manifest (`src/common/crt_manifest.c`)
- PEB command-line parsing and direct process entry (`snd_crtless_poc_entry`)
- Native backend profile without the Windows SDK
- Status-driven exit codes through `RtlExitUserProcess`

## Building

```bat
build.bat pocs crtless
:: → build32/pocs/Release/unified.exe, build64/pocs/Release/unified.exe
```

Equivalent raw CMake (single architecture):

```bash
cmake -B build -DSND_CRTLESS=ON -DSND_ENABLE_DEBUG=OFF -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
# → build/pocs/Release/unified.exe
# → build/Release/sindri_engine.lib
```

Use a **clean build directory** when switching between CRT-less and normal/test builds.

The PoC target adds:

| Setting | Effect |
|---|---|
| `src/frontend_crtless.c` in the source list | Replaces `main` with `snd_crtless_poc_entry` |
| `/NODEFAULTLIB /ENTRY:snd_crtless_poc_entry` | No CRT startup code |
| `/GS- /sdl- /Gs2147483647` | Runtime-check-free, no stack probing |
| `SND_CRTLESS=1` | Selects the no-op print adapter and native defaults |

The engine also compiles `src/common/crt_manifest.c` to supply intrinsic `memcpy`/`memset` fallbacks.

> [!WARNING]
> `SND_CRTLESS=ON` force-disables `SND_ENABLE_DEBUG` and `SND_USE_PRINTF`, and `SND_BUILD_TESTS=ON` force-disables `SND_CRTLESS`. CRTLESS builds that pull `<stdio.h>` into the status/debug paths will not link.

## Frontend behavior

`snd_crtless_poc_entry` reads the process command line from the PEB:

```c
SND_UNICODE_STRING *command_line = NULL;
snd_env_get_command_line(NULL, &command_line);
// tokenize into argc/argv, then:
int code = unified_main(argc, argv);

FARPROC exit_address = NULL;
if (SND_SUCCEEDED(snd_ntdll_get_active_export(SND_HASH_RTLEXITUSERPROCESS, &exit_address)))
    ((SND_RtlExitUserProcess_t)exit_address)((NTSTATUS)code);

for (;;) {
}
```

> [!NOTE]
> The infinite loop is the fallback if `RtlExitUserProcess` cannot be resolved from the active `ntdll`. The target is a PoC; production implants should provide their own teardown.

Tokenizer limits:

| Limit | Value |
|---|---|
| Arguments | 32 (`POC_MAX_ARGS`, last slot reserved for the NULL terminator) |
| Argument length | 512 bytes including the terminator |
| Encoding | ASCII; code points above `0x7F` become `?` |
| Quoting | `"` toggles whitespace grouping and is stripped from the token |

## Command compatibility

All commands and options work as documented in [cli.md](cli.md), with these differences:

- The default backend is `--nt`; `--win` is rejected during backend initialization with `SND_STATUS_INVALID_COMMAND_LINE_ARG`.
- The print adapter is a no-op, so usage text, progress logs, and error context are silent. Exit codes remain the only feedback channel.
- `unified hg` allocates its demo buffer through `snd_mem_nt` instead of `snd_mem_win`.
- `--nt`/`--sys` file operands (`-f`, `-t`) still require fully qualified paths.

## OpSec impact

- No CRT startup or default-library imports to fingerprint.
- PEB-only command-line retrieval; no console or `stdio` usage.
- Backend defaults to native `ntdll` calls, with `--sys` available for syscall-backed memory operations.

## See also

- [Building: CRT-less tier](../getting_started/building.md)
- [Common: CRT manifest](../common/infrastructure.md)
- [cli.md](cli.md) — options that remain available without the SDK
