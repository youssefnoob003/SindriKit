# Syscall Primitives

Direct and indirect syscall invocation bypasses hooked `ntdll.dll` stubs by resolving System Service Numbers (SSNs) and executing the `syscall` instruction from custom ASM stubs — either inline (direct) or by jumping to a legitimate NTDLL gadget (indirect).

**Header:** `include/sindri/primitives/syscalls.h`  
**Implementation:** `src/primitives/execution/syscalls/`

> [!WARNING]
> **Bootstrap required:** Call `snd_ntdll_set_clean()`, configure at least one strategy via `snd_syscall_set_resolver()`, and configure the invoker via `snd_syscall_set_invoker()` before `snd_syscall_invoke()`. For indirect invocation, also set a gadget finder via `snd_syscall_set_gadget_finder()`. With `SND_USE_DEFAULTS=ON`, only `snd_ntdll_set_clean()` is required — the resolver, invoker, and gadget finder are pre-configured at compile time. `_sys` memory/process/mapping backends depend on this pipeline.

## How `_sys` primitives use syscalls

```
snd_mem_sys / snd_proc_sys / snd_map_sys
        │
        ▼
  snd_syscall_invoke(func_hash, &args) <- encapsulates resolution and dispatch
        │
    ┌───┴───────────────────┐
    ▼                       ▼
  snd_syscall_direct_     snd_syscall_indirect_
  invoke_asm              invoke_asm
```

Function hashes come from `sindri_hashes.h` (build-generated, e.g. `SND_HASH_NTALLOCATEVIRTUALMEMORY`).

## Resolution strategies

| Function | Source | Approach |
|---|---|---|
| `snd_syscall_resolve_ssn_scan` | `resolvers/scan.c` | Read SSN from stub bytes; neighbor scan if hooked |
| `snd_syscall_resolve_ssn_sort` | `resolvers/sort.c` | Build sorted `Zw*` export table; index = SSN |

Typical registration:

```c
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
snd_syscall_set_invoker(snd_syscall_direct_invoke_asm);
// or for indirect:
// snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
// snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);
```

Custom resolvers matching `snd_syscall_resolver_t` can be added to the chain (max 4 strategies).

## Invocation modes

| Function | File | Description |
|---|---|---|
| `snd_syscall_direct_invoke_asm` | `src/primitives/execution/syscalls/invokers/direct_*.asm` | Executes `syscall`/`sysenter` inline |
| `snd_syscall_indirect_invoke_asm` | `src/primitives/execution/syscalls/invokers/indirect_*.asm` | Jumps to a legitimate NTDLL gadget |
| `snd_syscall_spoofed_invoke_asm` | `src/primitives/execution/syscalls/invokers/spoofed_*.asm` | Spoofs call stack with dynamic Fat Frame discovery |

> [!NOTE]
> Build with `SND_USE_DEFAULTS=ON` to pre-configure the invoker (`snd_syscall_indirect_invoke_asm`), gadget finder (`snd_syscall_find_gadget_scan`), and primary resolver (`snd_syscall_resolve_ssn_scan`) at compile time. Only `snd_ntdll_set_clean()` is then needed at runtime.


## Table of Contents

- [pipeline.md](pipeline.md) — lifecycle from bootstrap to kernel transition
- [engines.md](engines.md) — scan vs sort resolver internals
- [api_reference.md](../../api_reference.md) — full public API

## Related documentation

- [Execution domain](../execution/README.md) — shared `src/primitives/execution/` layout
- [Mapping](../mapping/README.md) — KnownDlls bootstrap for clean `ntdll`
- [Primitives domain](../README.md)
