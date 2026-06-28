# Process Techniques

The process subdomain defines how SindriKit interacts with a **remote** target process. The injection engine never calls `OpenProcess` or `WriteProcessMemory` directly; it routes all cross-process operations through `ctx->proc_api`.

## Paradigm 1: Win32 Process Operations (`snd_proc_win`)

The `snd_proc_win` implementation backs the process interface with standard Win32 APIs:

- **Open:** `OpenProcess`
- **Remote alloc:** `VirtualAllocEx`
- **Remote write:** `WriteProcessMemory`
- **Remote protect:** `VirtualProtectEx`
- **Remote thread:** `CreateRemoteThread`
- **Close:** `CloseHandle`

### OpSec Implications

Every operation generates high-visibility telemetry through `kernel32.dll` and hooked `ntdll.dll` stubs. Intended for diagnostics (`pocs/inject_shell/main.c` uses this profile) or environments without EDR.

## Paradigm 2: NT API (`snd_proc_nt`)

The `snd_proc_nt` implementation resolves NT functions from `ntdll.dll` via PEB walking and hash-based EAT parsing:

- **Open:** `NtOpenProcess`
- **Remote alloc:** `NtAllocateVirtualMemory`
- **Remote write:** `NtWriteVirtualMemory`
- **Remote protect:** `NtProtectVirtualMemory`
- **Remote thread:** `NtCreateThreadEx` (with `THREAD_ALL_ACCESS` / `0x1FFFFF`)
- **Close:** `NtClose`

### OpSec Implications

Bypasses `kernel32.dll` and avoids plaintext API strings. Calls still execute through in-process `ntdll.dll` stubs where inline EDR hooks may fire.

## Paradigm 3: Direct Syscalls (`snd_proc_sys`)

The `snd_proc_sys` implementation invokes the same NT operations through the syscall resolution pipeline and ASM stubs:

- **Open / Alloc / Write / Protect / Thread / Close:** Direct syscalls resolved by compile-time function hashes

Remote thread creation uses `NtCreateThreadEx` with the same access mask (`0x1FFFFF`) as the NT backend.

### OpSec Implications

Direct syscalls bypass userland EDR hooks on `ntdll.dll` stubs. This is the preferred backend for stealth injection profiles.

> [!WARNING]
> `snd_proc_sys` requires a bootstrapped syscall pipeline (`snd_syscall_set_ntdll`, `snd_syscall_strategy_set`, `snd_syscall_strategy_add`) before any `_sys` process call. See the [Syscall Primitives](../syscalls/README.md) documentation.

## Injection Integration

The classic injection chain injects a process backend into `snd_inj_ctx_t`:

```c
snd_inj_ctx_t inj_ctx = {0};
inj_ctx.target_pid = target_pid;
inj_ctx.proc_api   = &snd_proc_sys;

snd_status_t status = snd_inj_classic_pe(&ldr_ctx, &inj_ctx);
snd_inj_cleanup(&inj_ctx);
```

The injection engine progresses through discrete stages (`SND_INJ_STAGE_*`), calling `proc_api` callbacks at each step:

1. `open_process` — acquire a handle to `target_pid`
2. `alloc_remote` — reserve/commit memory in the remote process
3. `write_remote` — copy the prepared payload
4. `protect_remote` — set final page protections (e.g. `PAGE_EXECUTE_READ`)
5. `create_remote_thread` — execute the entry point
6. `close_handle` — release process and thread handles via `snd_inj_cleanup`

A typical full-stealth profile (`pocs/inject_pe/main.c`):

1. Map clean `ntdll.dll` from KnownDlls (`snd_om_knowndll_map` + `snd_map_nt`)
2. Bootstrap syscalls with the mapped base
3. Load payload locally with `snd_mem_sys` + `snd_mod_nt`
4. Inject remotely with `snd_proc_sys`

## Mixing Backends

Process backends are independent of memory and module backends. The local loader can use `snd_mem_sys` while injection uses `snd_proc_win`, or any other combination — each `snd_*_api_t` table is injected separately.
