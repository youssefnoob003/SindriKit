# Thread Techniques

The thread subdomain wraps the operations SindriKit performs on an existing thread handle. It is deliberately small: creation and process-level handles belong to the [process subdomain](../process/internals.md), while this table exists so injection techniques can queue, resume, and rewrite threads without hardcoding `QueueUserAPC`, `NtQueueApcThread`, or `NtGetContextThread`.

## Context access

`get_context` / `set_context` exchange a portable `SND_THREAD_REGISTERS`
projection (`ip/sp/cx/dx/rflags`) with the native `CONTEXT`. The projection is
defined in `sindri/primitives/thread.h`, and the entry-frame ABI macros (argument
placement, stack alignment, EFLAGS) live natively in the Hijack engine. The SDK-free NT/syscall backends
map via `offsetof`/`sizeof` on a field-for-field WINNT mirror, and the Win32
backend compiles compile-time parity asserts against the real `CONTEXT` so all
three backends agree on offsets. The engine never touches a native context.

## Paradigm 1: Win32 (`snd_thread_win`)

`src/primitives/thread/win.c` uses the documented Win32 APIs directly:

- **Queue APC:** `QueueUserAPC`
- **Resume:** `ResumeThread`
- **Suspend:** `SuspendThread`
- **Context:** `GetThreadContext` / `SetThreadContext`
- **Close:** `CloseHandle`

### OpSec implications

Every call routes through `kernel32` and hooked `ntdll` stubs. Suitable for diagnostics and the `--win` PoC profile.

## Paradigm 2: NT API (`snd_thread_nt`)

`src/primitives/thread/nt.c` resolves the NT equivalents from the **active** NTDLL via hash (`snd_ntdll_get_active_export`) and invokes them directly:

- **Queue APC:** `NtQueueApcThread` (`SND_HASH_NTQUEUEAPCTHREAD`)
- **Resume:** `NtResumeThread` (`SND_HASH_NTRESUMETHREAD`)
- **Suspend:** `NtSuspendThread` (`SND_HASH_NTSUSPENDTHREAD`)
- **Context:** `NtGetContextThread` / `NtSetContextThread`
  (`SND_HASH_NTGETCONTEXTTHREAD` / `SND_HASH_NTSETCONTEXTTHREAD`)
- **Close:** `NtClose` (`SND_HASH_NTCLOSE`)

No `kernel32` involvement and no plaintext API strings, but calls still execute through in-process `ntdll` stubs where inline hooks may fire.

## Paradigm 3: Direct Syscalls (`snd_thread_sys`)

`src/primitives/thread/sys.c` sends the same NT operations through `snd_syscall_invoke` with the matching hashes. Handle-closing tolerates `NULL`/`INVALID_HANDLE_VALUE` as a no-op.

> [!WARNING]
> Requires a bootstrapped syscall pipeline (see [Syscalls](../syscalls/README.md)). Without it, every call fails during SSN resolution.

## APC integration

The APC injection chain (`snd_inj_apc_*`) consumes this table after creating a suspended process through `snd_inj_apc_create_target`:

1. `thread_api->queue_apc(target_thread, remote_entry_point, remote_arg)` — arms the APC.
2. `thread_api->resume_thread(target_thread)` — resumes the suspended initial thread, triggering the APC.

`snd_inj_cleanup` closes the thread handle via `close_handle`.

## Hijack integration

The hijack injection chain (`snd_inj_hijack_*`) consumes this table after
creating a suspended process through `snd_inj_hijack_create_target`:

1. `thread_api->get_context(initial_thread, &live)` — capture live `sp`/`rflags`.
2. Paint the entry frame (engine, `SND_THREAD_REGISTERS`).
3. `thread_api->set_context(initial_thread, &frame)` — rewrite `ip/sp/cx/dx/rflags`.
4. `thread_api->resume_thread(initial_thread)` — suspend count 1→0, payload runs.

`snd_inj_cleanup` closes the thread handle via `close_handle`.

## Mixing backends

The thread table is injected independently of memory, module, and process tables. A common stealth profile pairs `snd_proc_sys` for remote memory with `snd_thread_sys` for the APC/resume step; the `--win` fallback uses `snd_thread_win` alongside `snd_proc_win`.

## See also

- [Process techniques](../process/internals.md) — `create_process` / `create_remote_thread`
- [Injection: APC](../../injection/internals.md#apc-technique-early-bird-snd_inj_apc_)
- [Syscalls](../syscalls/README.md) — bootstrap for `snd_thread_sys`
