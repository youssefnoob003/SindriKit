# Dependency Injection

SindriKit utilizes a strict Dependency Injection (DI) pattern to decouple offensive intent from execution mechanics. This is a foundational architectural choice that enables the toolkit to remain highly evasive and adaptable.

## The Problem with Hardcoded APIs

Traditional reflective loaders and red team tools often hardcode their system interactions. A typical loader will directly call `VirtualAlloc`, `GetProcAddress`, or their `Nt` equivalents (`NtAllocateVirtualMemory`). This creates several problems:
1. **Inflexibility**: Changing from Win32 APIs to direct syscalls requires significant refactoring.
2. **Signaturing**: Hardcoded API strings or import table entries create strong static signatures.
3. **Telemetry**: Direct calls to monitored APIs trigger userland hooks placed by EDRs.

## The SindriKit Approach

> [!IMPORTANT]
> SindriKit never makes direct OS calls from its core engine. Instead, it relies on function pointer tables injected into the operation's context.

These tables are defined in `include/sindri/primitives/os_api.h` via two primary structures:

### Memory Capabilities (`snd_memory_api_t`)
Defines the primitive operations for memory management:
- `alloc`: Allocate memory (e.g., `VirtualAlloc`, `NtAllocateVirtualMemory`)
- `free`: Free memory
- `protect`: Change memory protections

### Module Capabilities (`snd_module_api_t`)
Defines the primitive operations for module and import resolution:
- `load_library`: Load a module into the process
- `get_proc_address`: Resolve a function address
- `get_module_base`: Retrieve the base address of a loaded module

These tables also support hash-based resolution (e.g., `load_library_hash`) to avoid plaintext strings.

## Usage in Context

When initializing an operation, such as a reflective loader, the operator explicitly injects the desired OS APIs into the context:

```c
snd_loader_ctx_t ctx = {0};

// Inject standard Win32 APIs (for diagnostic/standard profiles)
ctx.mem_api = &snd_mem_win;
ctx.mod_api = &snd_mod_win;

// OR Inject direct syscall native APIs (for stealth profiles)
ctx.mem_api = &snd_mem_native;
ctx.mod_api = &snd_mod_native;
```

Because the `snd_loader_ctx_t` relies entirely on `ctx.mem_api` and `ctx.mod_api`, the underlying execution primitive can be swapped effortlessly without altering the core reflective loading logic. 

## Custom Implementations
Because the DI pattern is interface-based, advanced operators can define their own `snd_memory_api_t` instances. For example, memory allocations could be routed through a custom hypervisor, a secondary process, or highly specific ROP chains, all while utilizing the standard SindriKit loader engine.
