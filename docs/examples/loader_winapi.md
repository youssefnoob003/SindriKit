# PoC: loader_winapi

**Location:** `pocs/loader_winapi/`

This PoC loads a raw PE payload from disk using standard, fully documented Win32 APIs throughout. Every memory operation (`VirtualAlloc`, `VirtualProtect`) and every module operation (`LoadLibraryA`, `GetProcAddress`) hits the standard API layer and any EDR hooks installed on it. It is intended strictly for diagnostic use and pipeline validation, not operational deployment.

## What it demonstrates

- The baseline reflective loading pipeline: parse → alloc → map → relocate → resolve imports → protect → TLS → execute.
- That the context-based architecture works correctly before introducing evasion complexity.
- A clean baseline to diff against when debugging why a stealth variant fails.

## Walkthrough

No syscall bootstrapping is required. The setup reduces to injecting the Win32-backed API tables and calling the execution chain wrapper:

```c
// Read the raw PE payload from disk into a buffer
snd_buffer_t payload = { ... };

// Initialize the loader context with Win32 primitives
snd_loader_ctx_t ctx = {0};
ctx.raw_source = &payload;
ctx.mem_api    = &snd_mem_win;   // -> VirtualAlloc / VirtualProtect
ctx.mod_api    = &snd_mod_win;   // -> LoadLibraryA / GetProcAddress

// Run the full prepare chain (parse, alloc, map, relocate, IAT, protect, TLS)
snd_status_t s = snd_prepare_reflective_image(&ctx);
if (s.code != SND_SUCCESS) {
    snd_status_print(s);
    return 1;
}

// Execute the entry point
s = snd_execute_reflective_image(&ctx);
if (s.code != SND_SUCCESS) {
    snd_status_print(s);
}

// Optional cleanup
snd_free_mapped_image(&ctx);
```

**OpSec impact:** Every allocation and protection change is made via documented Win32 APIs. Any EDR with userland hooks on `VirtualAlloc` and `VirtualProtect` will see every operation in full. String artifacts from `LoadLibraryA` calls appear in the call stack. This profile is an absolute EDR flare.
