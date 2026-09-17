# Files: Techniques

The file capability is a single `load` callback that reads a whole file into a tracked `snd_buffer_t` and installs the allocator-matched release routine. Three backends implement the same contract; see [README.md](README.md) for the public usage.

---

## Paradigm 1: Win32 (`snd_file_win`)

`src/primitives/files/win.c`:

1. `CreateFileA(path, SND_GENERIC_READ, SND_FILE_SHARE_READ, …, SND_OPEN_EXISTING, SND_FILE_ATTRIBUTE_NORMAL, …)`
2. `GetFileSizeEx` — rejects empty files (`SND_STATUS_FILE_TOO_SMALL`) and files above `SND_MAX_DWORD` (`SND_STATUS_FILE_TOO_LARGE`).
3. `HeapAlloc(GetProcessHeap(), SND_HEAP_ZERO_MEMORY, size)`
4. `ReadFile` — the read must equal the queried size.

The buffer is released with `snd_buffer_free_win` (`HeapFree`). Relative and absolute paths are both accepted.

## Paradigm 2: NT API (`snd_file_nt`)

`src/primitives/files/nt.c` resolves `NtCreateFile` / `NtQueryInformationFile` / `NtReadFile` / `NtClose` through the **active** NTDLL (`snd_ntdll_get_active_export`):

1. Prefix the path with the Object Manager form `\??\` (`nt_build_path`) and build an `SND_UNICODE_STRING`.
2. `NtCreateFile` with `SND_GENERIC_READ | SND_SYNCHRONIZE`, `SND_FILE_OPEN`, `SND_FILE_NON_DIRECTORY_FILE | SND_FILE_SYNCHRONOUS_IO_NONALERT`.
3. `NtQueryInformationFile` (`SND_FILE_STANDARD_INFORMATION_CLASS`) for `EndOfFile`.
4. `snd_mem_nt.alloc` for the destination, then `NtReadFile`.

The buffer is released with `snd_buffer_free_nt` (`NtFreeVirtualMemory`).

> [!WARNING]
> Because the NT path prefix requires a fully qualified DOS path, relative paths are not resolved and fail with `STATUS_OBJECT_PATH_NOT_FOUND` (`0xC000003A`). See the [path constraint](README.md#backends).

## Paradigm 3: Direct Syscalls (`snd_file_sys`)

`src/primitives/files/sys.c` performs the same NT sequence, but every call goes through `snd_syscall_invoke` (`SND_HASH_NTCREATEFILE`, `SND_HASH_NTQUERYINFORMATIONFILE`, `SND_HASH_NTREADFILE`, `SND_HASH_NTCLOSE`). Destination memory comes from `snd_mem_sys`, and the buffer is released with `snd_buffer_free_sys`.

> [!WARNING]
> Requires a bootstrapped syscall pipeline (see [Syscalls](../syscalls/README.md)); absolute paths only, same as `snd_file_nt`.

## Buffer ownership

`load` pairs the allocator with a matching release callback via `snd_buffer_init`; `snd_buffer_free` later dispatches through `free_routine`:

| Backend | Allocation | Release callback |
|---|---|---|
| `snd_file_win` | `HeapAlloc` | `snd_buffer_free_win` (`HeapFree`) |
| `snd_file_nt` | `snd_mem_nt.alloc` | `snd_buffer_free_nt` (`NtFreeVirtualMemory`) |
| `snd_file_sys` | `snd_mem_sys.alloc` | `snd_buffer_free_sys` (syscall `NtFreeVirtualMemory`) |

Callers must use `snd_buffer_free`, not a backend-specific free, so the producing backend is honoured.

## Size and bounds

All backends reject `size == 0` and `size > SND_MAX_DWORD`, store the exact size in `buf.size`, and treat a short read as `SND_STATUS_FILE_READ_FAILED`. Downstream parsers inherit these bounds through `snd_buffer_bounds_check`.

## OpSec impact

| Backend | Telemetry |
|---|---|
| `snd_file_win` | `CreateFileA`/`ReadFile` through `kernel32` and hooked `ntdll` stubs |
| `snd_file_nt` | Native calls still pass through in-process `ntdll` stubs (inline hooks may fire) |
| `snd_file_sys` | File I/O bypasses userland stubs; disk reads remain visible to kernel/AV telemetry |

Production implants can bypass file loading entirely and supply a pre-allocated `snd_buffer_t` to the loader.

## See also

- [Files primitive](README.md)
- [Memory techniques](../memory/internals.md) — allocators used by the `nt`/`sys` backends
- [Syscalls](../syscalls/README.md) — bootstrap required by `snd_file_sys`
