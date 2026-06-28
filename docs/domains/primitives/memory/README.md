# Memory Primitives

Local virtual memory management: allocation, protection changes, and deallocation in the current process. All operations route through an injected `snd_memory_api_t` table.

> [!CAUTION]
> **OpSec:** `snd_mem_win` routes through userland hooks in `kernel32` / `ntdll`. Prefer `snd_mem_nt` or `snd_mem_sys` for evasive deployments.

## Header map

| Header | Role |
|---|---|
| `sindri/primitives/memory.h` | Pre-built instances: `snd_mem_win`, `snd_mem_nt`, `snd_mem_sys` |
| `sindri/primitives/os_api.h` | `snd_memory_api_t` callback typedefs |

## Source map

| Source | Backend |
|---|---|
| `src/primitives/memory/win.c` | `snd_mem_win` |
| `src/primitives/memory/nt.c` | `snd_mem_nt` |
| `src/primitives/memory/sys.c` | `snd_mem_sys` |

## Table of Contents

- [techniques.md](techniques.md) — Win32 vs NT vs syscall paradigms
- [api_reference.md](api_reference.md) — `snd_memory_api_t` and instances

## Related documentation

- [Primitives domain](../README.md)
- [Syscalls](../syscalls/README.md) — bootstrap required for `snd_mem_sys`
