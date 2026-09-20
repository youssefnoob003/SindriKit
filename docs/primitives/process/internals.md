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

Every operation generates high-visibility telemetry through `kernel32.dll` and hooked `ntdll.dll` stubs. Intended for diagnostics (`unified inject classic shell ... --win`) or environments without EDR.

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
> `snd_proc_sys` requires a bootstrapped syscall pipeline (`snd_ntdll_set_clean`, `snd_syscall_set_resolver`, `snd_syscall_set_invoker`) before any `_sys` process call. See the [Syscall Primitives](../syscalls/README.md) documentation.

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
6. `free_remote` — best-effort release of pre-execution remote allocations
7. `terminate_process` — abort a target created by APC/hijack cleanup when needed
8. `close_handle` — release process and thread handles via `snd_inj_cleanup`

A typical full-stealth profile (`unified inject classic ... --sys`):

1. Map clean `ntdll.dll` from KnownDlls (`snd_om_knowndll_map` + `snd_map_nt`)
2. Bootstrap syscalls with the mapped base
3. Load payload locally with `snd_mem_sys` + `snd_mod_nt`
4. Inject remotely with `snd_proc_sys`

## Mixing Backends

Process backends are independent of memory and module backends. The local loader can use `snd_mem_sys` while injection uses `snd_proc_win`, or any other combination — each `snd_*_api_t` table is injected separately.

---

## Known Limitation: CSRSS Registration and GUI Targets

> [!CAUTION]
> `snd_proc_nt` and `snd_proc_sys` create child processes via `NtCreateUserProcess` directly — bypassing the Win32 `CreateProcessW` wrapper entirely. This makes **GUI-heavy executables** (e.g. `notepad.exe`, `mspaint.exe`) **unsuitable as APC injection targets** when using either of these backends.

### Symptom

The child process is created and the APC fires, but the target process crashes during initialization with an error such as:

```
The ordinal 345 could not be located in the dynamic link library
C:\Windows\SysWOW64\notepad.exe
```

Simple or lightweight executables work fine because they do not exercise the full Win32 subsystem.

### Root Cause (Speculative)

`CreateProcessW` performs significant post-kernel work that `NtCreateUserProcess` alone does not:

1. **CSRSS notification** — `kernel32!CreateProcessInternalW` calls `CsrClientCallServer` (with `BaseSrvCreateProcess`) to register the new process with the Client/Server Runtime SubSystem. Without this registration, the child process has no valid connection to the Win32 subsystem.
2. **SxS Activation Context** — `CreateProcessW` sets up Side-by-Side assembly manifests. GUI apps that depend on Common Controls v6, MSVC redistributables, or other manifested assemblies will fail to resolve dependencies without this context.
3. **Subsystem DLL initialisation** — When the suspended thread is resumed and `ntdll!LdrInitializeThunk` runs the loader, GUI DLLs (`user32.dll`, `gdi32.dll`) attempt to connect to CSRSS via `UserClientDllInitialize`. The unregistered process causes this connection to fail, which cascades into loader errors (ordinal resolution failures, import snap failures) as dependent DLLs abort their `DllMain` calls.

The ordinal error message is a secondary symptom: a DLL's `DllMain` failed or was skipped, leaving an export table partially initialised. A later import resolution then fails to find the expected ordinal in the broken module chain.

### Workaround

Use lightweight console executables or minimal launcher stubs as APC targets when operating in `--nt` or `--sys` mode. If the injection target must be a GUI application, fall back to `--win` mode, which uses `CreateProcessW` and performs the full CSRSS registration path.

### Future Work

If the injection target is a GUI application, the full CSRSS registration path must be performed. I invite contributions to SindriKit to implement this functionality, but for now, I'm leaving it as a known limitation.
