# Memory Techniques

The memory subdomain defines how SindriKit allocates, protects, and frees virtual memory in the **local** process. Because of the framework's Dependency Injection architecture, a reflective loader or other domain never hardcodes `VirtualAlloc`; it calls `ctx->mem_api->alloc` with whatever backend the operator injected during initialization.

## Paradigm 1: Win32 Allocation (`snd_mem_win`)

The `snd_mem_win` implementation backs the `snd_memory_api_t` interface using standard Win32 functions from `kernel32.dll`:

- **Allocation:** `VirtualAlloc`
- **Protection:** `VirtualProtect`
- **Deallocation:** `VirtualFree`

### OpSec Implications

This paradigm provides maximum stability but generates significant telemetry. Every call routes through `kernel32.dll` into the userland `ntdll.dll` stub for `NtAllocateVirtualMemory`, triggering inline hooks placed by security products.

Intended for diagnostic scenarios, non-evasive toolsets, or environments where EDR instrumentation is known to be absent.

## Paradigm 2: NT API (`snd_mem_nt`)

The `snd_mem_nt` implementation resolves `NtAllocateVirtualMemory`, `NtFreeVirtualMemory`, and `NtProtectVirtualMemory` by walking the PEB to locate `ntdll.dll` and parsing its Export Address Table via compile-time hashes. Resolved function pointers are invoked directly.

- **Allocation:** `NtAllocateVirtualMemory` on `GetCurrentProcess()` (handle `-1`)
- **Protection:** `NtProtectVirtualMemory`
- **Deallocation:** `NtFreeVirtualMemory`

### OpSec Implications

This paradigm bypasses `kernel32.dll` and avoids `GetProcAddress` / plaintext API strings. However, calls still execute through in-process `ntdll.dll` stubs; inline EDR hooks on those stubs will still fire.

## Paradigm 3: Direct Syscalls (`snd_mem_sys`)

The `snd_mem_sys` implementation bypasses both `kernel32.dll` and hooked `ntdll.dll` stubs by resolving System Service Numbers (SSNs) and executing the `syscall` instruction from custom ASM stubs.

- **Allocation:** Direct syscall to `NtAllocateVirtualMemory`
- **Protection:** Direct syscall to `NtProtectVirtualMemory`
- **Deallocation:** Direct syscall to `NtFreeVirtualMemory`

Local operations use `(HANDLE)-1` as the process handle, matching the NT convention for the current process.

### OpSec Implications

Direct syscalls bypass userland EDR hooks inside `ntdll.dll` stubs. The kernel receives memory requests without userland telemetry sensors recording the API transition.

> [!WARNING]
> `snd_mem_sys` requires the operator to bootstrap the syscall pipeline before invocation:
>

> ```c
> snd_syscall_set_ntdll(clean_ntdll_base);
> snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
> snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
> snd_syscall_set_invoker(snd_syscall_direct_invoke_asm);
> // or for indirect syscalls:
> // snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
> // snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);
> ```
> If the pipeline is not configured, syscall resolution fails immediately and all `_sys` memory calls return an error.

## Mixing Backends

Backends are composable across domains. A common stealth loader profile injects `snd_mem_sys` for allocations while using `snd_mod_nt` for import resolution — see `pocs/inject_pe/main.c`. Local NT-only loading without direct syscalls uses `snd_mem_nt` + `snd_mod_nt` as in `pocs/loader_nowinapi/main.c`.
