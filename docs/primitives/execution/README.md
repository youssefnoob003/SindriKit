# Execution Primitives

Low-level mechanisms for **calling unknown function signatures**, **crossing WoW64 bitness boundaries**, and (in source layout) hosting the **syscall invoker**. These sit below memory, module, and process backends — loaders and injection chains consume them indirectly.

## Header map

| Header | Role |
|---|---|
| `sindri/primitives/ffi.h` | `snd_ffi_execute` — dynamic call bridge for resolved exports |
| `sindri/primitives/heavens_gate.h` | `snd_is_wow64`, `snd_hg_execute_64` — WoW64 → native x64 |
| `sindri/primitives/syscalls.h` | SSN resolution pipeline + `snd_syscall_direct_invoke_asm` / `snd_syscall_indirect_invoke_asm` (documented under [syscalls/](../syscalls/README.md)) |

All three are pulled in by `include/sindri/primitives.h` and `include/sindri.h`.

## Source layout

Implementation lives under `src/primitives/execution/`:

```
src/primitives/execution/
├── ffi/
│   ├── ffi_invoke.c          <- snd_ffi_execute validation + dispatch
│   └── asm/
│       ├── ffi_x64.asm       <- snd_ffi_bridge_x64
│       └── ffi_x86.asm       <- snd_ffi_bridge_x86
├── heavens_gate/
│   ├── heavens_gate.c        <- WoW64 detection + argument padding
│   └── asm/
│       └── heavens_gate_x86.asm  <- snd_hg_invoke_x86 (CS 0x33 transition)
└── syscalls/
    ├── pipeline.c             <- strategy chain and orchestration
    ├── resolvers/
    │   ├── scan.c             <- snd_syscall_resolve_ssn_scan
    │   └── sort.c             <- snd_syscall_resolve_ssn_sort
    ├── finders/
    │   ├── gadget.c           <- snd_syscall_find_gadget_scan
    │   └── spoof.c            <- snd_syscall_find_spoof_scan
    ├── status.c               <- syscall status descriptions
    └── invokers/
        ├── direct_x64.asm     <- direct x64 syscall stub
        ├── direct_x86.asm     <- direct x86 syscall stub
        ├── indirect_x64.asm   <- indirect x64 syscall stub
        ├── indirect_x86.asm   <- indirect x86 syscall stub
        └── spoofed_*.asm      <- spoofed syscall stubs
```

Syscall resolution and invocation are documented in the dedicated [syscalls](../syscalls/README.md) subtree; this directory focuses on FFI and Heaven's Gate.

## When to use which primitive

| Need | API | Typical consumer |
|---|---|---|
| Call a named export with unknown arity (`-e`/`-a`) | `snd_ffi_execute` | unified `load pe` command |
| Invoke 64-bit code from a 32-bit WoW64 process | `snd_hg_execute_64` | unified `hg` command on x86 |
| Bypass hooked `ntdll` stubs for memory/process ops | `snd_syscall_invoke` + `_sys` backends | unified `--sys` modes, `snd_mem_sys`, `snd_proc_sys` |

DllMain, TLS, and EXE entry use **typed direct calls** inside `snd_ldr_pe_execute_image` — not FFI. See [ffi.md](ffi.md).

## Table of Contents

- [ffi.md](ffi.md) — MASM bridges, calling conventions, C4152-safe type punning
- [heavens_gate.md](heavens_gate.md) — segment selector `0x33`, WoW64 TEB probe
- [api_reference.md](../../api_reference.md) — public FFI and Heaven's Gate signatures

## Related documentation

- [Syscalls domain](../syscalls/README.md) — SSN engines, bootstrap, `snd_syscall_direct_invoke_asm` / `snd_syscall_indirect_invoke_asm`
- [Loaders: DLL export FFI](../../loaders/internals.md)
- [PoC: heavens_gate](../../examples/heavens_gate.md)
