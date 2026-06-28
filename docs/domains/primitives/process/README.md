# Process Primitives

**Remote** process interaction: open target, allocate/write/protect remote memory, create a remote thread. Operations route through an injected `snd_process_api_t` table.

Primary consumer: injection domain (`snd_inj_ctx_t.proc_api`).

> [!CAUTION]
> **OpSec:** `snd_proc_win` routes cross-process operations through monitored Win32 APIs. Prefer `snd_proc_sys` or `snd_proc_nt` for evasive deployments.

## Header map

| Header | Role |
|---|---|
| `sindri/primitives/process.h` | `snd_proc_win`, `snd_proc_nt`, `snd_proc_sys` |
| `sindri/primitives/os_api.h` | `snd_process_api_t` callback typedefs |

## Source map

| Source | Backend |
|---|---|
| `src/primitives/process/win.c` | `snd_proc_win` |
| `src/primitives/process/nt.c` | `snd_proc_nt` |
| `src/primitives/process/sys.c` | `snd_proc_sys` |

## Table of Contents

- [techniques.md](techniques.md) — Win32 vs NT vs syscall paradigms, injection integration
- [api_reference.md](api_reference.md) — `snd_process_api_t` and instances

## Related documentation

- [Injection domain](../../injection/README.md)
- [Syscalls](../syscalls/README.md) — bootstrap for `snd_proc_sys`
- [Primitives domain](../README.md)
