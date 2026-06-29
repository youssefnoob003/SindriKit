# Mapping Techniques

The mapping subdomain exposes section operations through the `snd_mapping_api_t` function pointer table. Like memory and process primitives, the framework never hardcodes `OpenFileMappingW` or `NtOpenSection`; callers inject the desired backend at initialization time.

## Paradigm 1: Win32 Mapping (`snd_map_win`)

The `snd_map_win` implementation backs the mapping interface with standard Win32 APIs from `kernel32.dll`:

- **Open:** `OpenFileMappingW` with `FILE_MAP_READ`
- **View:** `MapViewOfFile` followed by `VirtualQuery` to determine the mapped region size
- **Close:** `CloseHandle`

### Path Constraints

The Win32 subsystem prepends its own namespace to section names. This backend is suitable for conventional named mappings (`Local\...`, `Global\...`) but **must not** be used for absolute NT Object Manager paths such as `\KnownDlls\ntdll.dll`. Those paths require `snd_map_nt` or `snd_map_sys`.

### OpSec Implications

Every call routes through userland hooks in `kernel32.dll` and `ntdll.dll`. Use this backend only for diagnostics or environments where EDR instrumentation is absent.

## Paradigm 2: NT API (`snd_map_nt`)

The `snd_map_nt` implementation resolves `NtOpenSection`, `NtMapViewOfSection`, and `NtClose` from `ntdll.dll` via PEB walking and hash-based Export Address Table (EAT) parsing. No `GetProcAddress` or plaintext API strings are required.

- **Open:** `NtOpenSection` with `SECTION_MAP_READ` and case-insensitive object attributes
- **View:** `NtMapViewOfSection` into `GetCurrentProcess()` with `PAGE_READONLY`
- **Close:** `NtClose`

### OpSec Implications

This paradigm bypasses `kernel32.dll` but still executes through in-process `ntdll.dll` stubs. Inline EDR hooks on those stubs will still fire. It is the typical choice for KnownDlls bootstrapping when the syscall pipeline is not yet configured.

## Paradigm 3: Direct Syscalls (`snd_map_sys`)

The `snd_map_sys` implementation invokes `NtOpenSection`, `NtMapViewOfSection`, and `NtClose` through the framework's syscall resolution pipeline and custom ASM stubs.

- **Open / View / Close:** Direct syscalls resolved by compile-time function hashes (`SND_HASH_NTOPENSECTION`, etc.)

### OpSec Implications

Direct syscalls bypass userland EDR hooks placed inside `ntdll.dll` stubs. This is the preferred backend for stealth KnownDlls mapping once the syscall pipeline is bootstrapped.

> [!WARNING]
> `snd_map_sys` requires the operator to configure the syscall pipeline (`snd_syscall_set_ntdll`, `snd_syscall_set_resolver`, `snd_syscall_set_invoker`) before invocation. Unconfigured pipelines cause immediate resolution failures.

## KnownDlls Bootstrapping

The Object Manager helper `snd_om_knowndll_map()` wraps a mapping backend to map a clean system DLL from the `\KnownDlls` (x64) or `\KnownDlls32` (x86) directory:

```c
PVOID clean_ntdll = NULL;
snd_status_t status = snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &clean_ntdll);
if (SND_FAILED(status)) {
    return status;
}

// Feed the mapped image into the syscall pipeline
snd_syscall_set_ntdll(clean_ntdll);
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
snd_syscall_set_invoker(snd_syscall_direct_invoke_asm);
// or for indirect syscalls:
// snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
// snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);
```

The helper builds the full Object Manager path (`SND_TARGET_KNOWNDLLS_DIR` + DLL name), calls `open` and `view`, then closes the section handle. The mapped view remains valid after the handle is closed.

Typical stealth profile (see `pocs/inject_pe/main.c`):

1. Map clean `ntdll.dll` from KnownDlls via `snd_map_nt`
2. Bootstrap the syscall pipeline with the mapped base
3. Switch remaining operations to `_sys` backends (`snd_mem_sys`, `snd_proc_sys`)
