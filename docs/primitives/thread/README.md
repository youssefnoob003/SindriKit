# Thread Primitives

Operations on an **existing** thread handle: queue an APC, resume, suspend, and close. All work routes through an injected `snd_thread_api_t` table.

Primary consumer: APC injection (`snd_inj_ctx_t.thread_api`) — see [injection internals](../../injection/internals.md).

> [!NOTE]
> Remote thread **creation** is not here. It lives on the process table as `create_remote_thread` (`snd_process_api_t`); this table operates on handles that already exist.

## Header map

| Header | Role |
|---|---|
| `sindri/primitives/thread.h` | Pre-built instances: `snd_thread_win`, `snd_thread_nt`, `snd_thread_sys` |
| `sindri/primitives/os_api.h` | `snd_thread_api_t` callback typedefs |

## API table

| Callback | Role |
|---|---|
| `queue_apc` | Queue an asynchronous procedure call to a thread |
| `resume_thread` | Decrement a thread's suspend count |
| `suspend_thread` | Increment a thread's suspend count |
| `close_handle` | Release a thread handle |

## Source map

| Source | Backend |
|---|---|
| `src/primitives/thread/win.c` | `snd_thread_win` |
| `src/primitives/thread/nt.c` | `snd_thread_nt` |
| `src/primitives/thread/sys.c` | `snd_thread_sys` |

## Table of Contents

- [internals.md](internals.md) — Win32 vs NT vs syscall paradigms, APC integration
- [api_reference.md](../../api_reference.md) — `snd_thread_api_t` and instances

## Related documentation

- [Process primitives](../process/README.md) — remote thread creation and process handles
- [Injection domain](../../injection/README.md) — APC consumer
- [Primitives domain](../README.md)
