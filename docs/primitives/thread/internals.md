# Thread Techniques

The thread subdomain wraps the four operations SindriKit performs on an existing thread handle. It is deliberately small: creation and process-level handles belong to the [process subdomain](../process/internals.md), while this table exists so injection techniques can queue and resume without hardcoding `QueueUserAPC` or `NtQueueApcThread`.

## Paradigm 1: Win32 (`snd_thread_win`)

`src/primitives/thread/win.c` uses the documented Win32 APIs directly:

- **Queue APC:** `QueueUserAPC`
- **Resume:** `ResumeThread`
- **Suspend:** `SuspendThread`
- **Close:** `CloseHandle`

### OpSec implications

Every call routes through `kernel32` and hooked `ntdll` stubs. Suitable for diagnostics and the `--win` PoC profile.

## Paradigm 2: NT API (`snd_thread_nt`)

`src/primitives/thread/nt.c` resolves the NT equivalents from the **active** NTDLL via hash (`snd_ntdll_get_active_export`) and invokes them directly:

- **Queue APC:** `NtQueueApcThread` (`SND_HASH_NTQUEUEAPCTHREAD`)
- **Resume:** `NtResumeThread` (`SND_HASH_NTRESUMETHREAD`)
- **Suspend:** `NtSuspendThread` (`SND_HASH_NTSUSPENDTHREAD`)
- **Close:** `NtClose` (`SND_HASH_NTCLOSE`)

No `kernel32` involvement and no plaintext API strings, but calls still execute through in-process `ntdll` stubs where inline hooks may fire.

## Paradigm 3: Direct Syscalls (`snd_thread_sys`)

`src/primitives/thread/sys.c` sends the same NT operations through `snd_syscall_invoke` with the matching hashes. Handle-closing tolerates `NULL`/`INVALID_HANDLE_VALUE` as a no-op.

> [!WARNING]
> Requires a bootstrapped syscall pipeline (see [Syscalls](../syscalls/README.md)). Without it, every call fails during SSN resolution.

## APC integration

The APC injection chain (`snd_inj_apc_*`) consumes this table after creating a suspended process through `proc_api->create_process`:

1. `thread_api->queue_apc(target_thread, remote_entry_point, remote_arg)` — arms the APC.
2. `thread_api->resume_thread(target_thread)` — resumes the suspended initial thread, triggering the APC.

`snd_inj_cleanup` closes the thread handle via `close_handle`.

## Mixing backends

The thread table is injected independently of memory, module, and process tables. A common stealth profile pairs `snd_proc_sys` for remote memory with `snd_thread_sys` for the APC/resume step; the `--win` fallback uses `snd_thread_win` alongside `snd_proc_win`.

## See also

- [Process techniques](../process/internals.md) — `create_process` / `create_remote_thread`
- [Injection: APC](../../injection/internals.md#apc-technique-early-bird-snd_inj_apc_)
- [Syscalls](../syscalls/README.md) — bootstrap for `snd_thread_sys`
