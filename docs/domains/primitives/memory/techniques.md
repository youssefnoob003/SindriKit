# Memory Techniques

The memory subdomain defines the core paradigms utilized by SindriKit to allocate, protect, and free virtual memory. Because of the framework's strict Dependency Injection architecture, an operation like a reflective loader never hardcodes how memory is acquired; it relies entirely on the `snd_memory_api_t` interface injected during initialization.

## Paradigm 1: Win32 Allocation (`snd_mem_win`)

The `snd_mem_win` implementation backs the `snd_memory_api_t` interface using standard, high-level Win32 functions exported by `kernel32.dll`.

- **Allocation:** `VirtualAlloc`
- **Protection:** `VirtualProtect`
- **Deallocation:** `VirtualFree`

### OpSec Implications
This paradigm provides maximum stability but generates massive telemetry surface. Every call to `VirtualAlloc` acts as a flare to AV/EDR sensors. The request is routed through `kernel32.dll` down into the userland `ntdll.dll` stub for `NtAllocateVirtualMemory`, directly triggering any inline hooks placed by security products.

This implementation is intended strictly for diagnostic scenarios, non-evasive toolsets, or environments where EDR instrumentation is known to be absent.

## Paradigm 2: Direct Syscalls (`snd_mem_native`)

The `snd_mem_native` implementation bypasses `kernel32.dll` and `ntdll.dll` entirely by utilizing the framework's internal Syscall Resolution pipeline.

- **Allocation:** Direct syscall to `NtAllocateVirtualMemory`
- **Protection:** Direct syscall to `NtProtectVirtualMemory`
- **Deallocation:** Direct syscall to `NtFreeVirtualMemory`

### OpSec Implications
By executing the `syscall` instruction directly, the implant cleanly bypasses userland EDR hooks placed inside the `ntdll.dll` stubs. The kernel receives the memory request without userland telemetry sensors ever recording the event.

> [!WARNING]
> `snd_mem_native` requires the operator to manually configure the syscall pipeline (via `snd_set_syscall_strategy` and `snd_set_ntdll`) prior to invocation. If the pipeline is not bootstrapped, the native memory calls will immediately fail.
