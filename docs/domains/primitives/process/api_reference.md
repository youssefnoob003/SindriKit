# Process: API Reference

This page documents the remote process capabilities exported by the framework. Type definitions live in `include/sindri/primitives/os_api.h`; pre-built instances are declared in `include/sindri/primitives/process.h`.

---

## Process API Instances (`sindri/primitives/process.h`)

Three pre-built instances of the `snd_process_api_t` interface are exported globally. These pointers are typically assigned to `ctx.proc_api` during injection initialization.

| Symbol | Backend | Source |
|---|---|---|
| `snd_proc_win` | Win32 API (`OpenProcess`, `VirtualAllocEx`, `WriteProcessMemory`, `VirtualProtectEx`, `CreateRemoteThread`, `CloseHandle`) | `src/primitives/process/win.c` |
| `snd_proc_nt` | NT API via PEB + EAT resolution (`NtOpenProcess`, `NtAllocateVirtualMemory`, `NtWriteVirtualMemory`, `NtProtectVirtualMemory`, `NtCreateThreadEx`, `NtClose`) | `src/primitives/process/nt.c` |
| `snd_proc_sys` | Direct syscalls via SSN resolution + ASM stub | `src/primitives/process/sys.c` |

---

## The Interface (`sindri/primitives/os_api.h`)

### `snd_process_api_t`

Function pointer table defining the remote process operations contract.

| Field | Signature | Description |
|---|---|---|
| `open_process` | `snd_status_t (*)(DWORD pid, DWORD desired_access, HANDLE *out_process)` | Opens a handle to the target process |
| `alloc_remote` | `snd_status_t (*)(HANDLE process, SIZE_T size, DWORD alloc_type, DWORD protect, PVOID *out_address)` | Allocates memory in the remote process |
| `write_remote` | `snd_status_t (*)(HANDLE process, PVOID base, const void *buffer, SIZE_T size, SIZE_T *bytes_written)` | Writes data into the remote process |
| `protect_remote` | `snd_status_t (*)(HANDLE process, PVOID base, SIZE_T size, DWORD new_protect, DWORD *old_protect)` | Changes memory protections in the remote process |
| `create_remote_thread` | `snd_status_t (*)(HANDLE process, PVOID start, PVOID param, HANDLE *out_thread)` | Creates a thread in the remote process |
| `close_handle` | `snd_status_t (*)(HANDLE handle)` | Closes a handle |

#### `open_process`

| Parameter | Description |
|---|---|
| `pid` | Target process ID |
| `desired_access` | Access mask (e.g. `PROCESS_ALL_ACCESS` or specific flags) |
| `out_process` | Receives the process handle on success |

**Status codes:** `SND_STATUS_PROCESS_OPEN_FAILED`, `SND_STATUS_INVALID_PARAMETER`

#### `alloc_remote`

| Parameter | Description |
|---|---|
| `process` | Handle to the target process |
| `size` | Number of bytes to allocate |
| `alloc_type` | Win32 allocation flags (`MEM_COMMIT`, `MEM_RESERVE`) |
| `protect` | Initial page protection |
| `out_address` | Receives the remote base address on success |

All backends allocate at an OS-chosen base (`NULL` hint). The Win32 backend uses `VirtualAllocEx`; NT and syscall backends use `NtAllocateVirtualMemory`.

**Status codes:** `SND_STATUS_VIRTUAL_ALLOC_FAILED`, `SND_STATUS_INVALID_PARAMETER`

#### `write_remote`

| Parameter | Description |
|---|---|
| `process` | Handle to the target process |
| `base_address` | Remote destination address |
| `buffer` | Local source buffer |
| `size` | Number of bytes to write |
| `bytes_written` | Optional; receives the number of bytes actually written |

**Status codes:** `SND_STATUS_VIRTUAL_WRITE_FAILED`, `SND_STATUS_INVALID_PARAMETER`

#### `protect_remote`

| Parameter | Description |
|---|---|
| `process` | Handle to the target process |
| `base_address` | Remote base address |
| `size` | Region size |
| `new_protect` | Desired page protection |
| `old_protect` | Optional; receives the previous protection |

**Status codes:** `SND_STATUS_VIRTUAL_PROTECT_FAILED`, `SND_STATUS_INVALID_PARAMETER`

#### `create_remote_thread`

| Parameter | Description |
|---|---|
| `process` | Handle to the target process |
| `start_address` | Remote thread entry point |
| `parameter` | Argument passed to the entry point |
| `out_thread` | Receives the thread handle on success |

NT and syscall backends use `NtCreateThreadEx` with `THREAD_ALL_ACCESS` (`0x1FFFFF`). The Win32 backend uses `CreateRemoteThread`.

**Status codes:** `SND_STATUS_THREAD_CREATE_FAILED`, `SND_STATUS_INVALID_PARAMETER`

#### `close_handle`

Closes a process or thread handle. The NT backend treats a `NULL` handle as success.

**Status codes:** `SND_STATUS_HANDLE_CLOSE_FAILED`

---

## Injection Context (`sindri/injection/context.h`)

Process backends are injected into the injection context:

```c
typedef struct _snd_inj_ctx_t {
    DWORD  target_pid;
    HANDLE target_process;
    PVOID  remote_base;
    PVOID  remote_entry_point;
    SIZE_T remote_size;
    HANDLE remote_thread;
    snd_inj_stage_t stage;
    const snd_buffer_t      *payload;
    const snd_process_api_t *proc_api;
} snd_inj_ctx_t;
```

Call `snd_inj_cleanup(&ctx)` to close handles and reset state after an injection attempt.

See the [Injection Domain](../../injection/README.md) for the full stage machine and chain orchestration.
