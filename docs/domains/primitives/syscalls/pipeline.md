# Cascading Syscall Pipeline

Windows EDRs instrument the kernel boundary by placing inline hooks inside `ntdll.dll` userland syscall stubs. When an implant calls `NtAllocateVirtualMemory`, it hits the hooked stub before reaching the kernel. The EDR inspects arguments, the call stack, and the execution context before deciding whether to allow or block the transition. 

Direct syscall invocation bypasses this entirely: if the operator knows the System Service Number (SSN) for the target function, the `syscall` instruction can be issued directly, transitioning straight to the kernel.

The challenge is reliable SSN resolution. The SSN for any given NT function is not stable across Windows versions or patch levels. It must be extracted at runtime from the in-memory `ntdll.dll` image. SindriKit implements four distinct strategies (Gates) for this, arranged into a user-configured cascading fallback pipeline.

## The Pipeline Design

SindriKit composes the various gate strategies into a single ordered pipeline. 

- `snd_set_syscall_strategy()` sets the primary strategy and resets any existing chain. 
- `snd_add_syscall_strategy()` appends fallbacks (maximum chain depth: 4). 
- When `snd_resolve_syscall()` is called, it attempts each strategy in registration order and returns the first successful result. A strategy that returns any error causes an immediate retry with the next entry in the pipeline.

### Full-Coverage Implementation
A full-coverage pipeline that handles unhooked, lightly hooked, and aggressively hooked environments looks like this:

```c
snd_set_syscall_strategy(snd_hell_extract_syscall);
snd_add_syscall_strategy(snd_halo_extract_syscall);
snd_add_syscall_strategy(snd_tartarus_extract_syscall);
snd_add_syscall_strategy(snd_veles_extract_syscall);
```

If all four strategies fail for a given function, `snd_resolve_syscall()` returns `SND_STATUS_SSN_NOT_FOUND`.

> [!IMPORTANT]
> `snd_set_ntdll()` (from `sindri/primitives/modules.h`) **must** be called before any resolution attempt. There is no implicit fallback. Feed it either the PEB-resident `ntdll.dll` base or, for a cleaner baseline, a base obtained via the KnownDlls mapping technique.
