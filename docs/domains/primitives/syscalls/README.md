# Syscall Primitives

Direct syscall invocation bypasses hooked `ntdll.dll` stubs by resolving System Service Numbers (SSNs) and executing the `syscall` instruction from custom ASM stubs.

**Header:** `include/sindri/primitives/syscalls.h`  
**Implementation:** `src/primitives/execution/syscalls/`

> [!WARNING]
> **Bootstrap required:** Call `snd_syscall_set_ntdll()` and configure at least one strategy via `snd_syscall_strategy_set()` before `snd_syscall_resolve()`. `_sys` memory/process/mapping backends depend on this pipeline.

## How `_sys` primitives use syscalls

```
snd_mem_sys / snd_proc_sys / snd_map_sys
        │
        ▼
  snd_syscall_resolve(func_hash)  ← cascading strategy chain
        │
        ▼
  snd_syscall_invoke_asm(&args)   ← x64/x86 ASM stub
```

Function hashes come from `sindri_hashes.h` (build-generated, e.g. `SND_HASH_NTALLOCATEVIRTUALMEMORY`).

## Resolution strategies

| Function | Source | Approach |
|---|---|---|
| `snd_syscall_resolve_ssn_scan` | `syscalls_scan.c` | Read SSN from stub bytes; neighbor scan if hooked |
| `snd_syscall_resolve_ssn_sort` | `syscalls_sort.c` | Build sorted `Zw*` export table; index = SSN |

Typical registration — scan first, sort as fallback:

```c
snd_syscall_strategy_set(snd_syscall_resolve_ssn_scan);
snd_syscall_strategy_add(snd_syscall_resolve_ssn_sort);
```

Custom resolvers matching `snd_syscall_resolver_t` can be added to the chain (max 4 strategies).

## Table of Contents

- [pipeline.md](pipeline.md) — lifecycle from bootstrap to kernel transition
- [engines.md](engines.md) — scan vs sort resolver internals
- [api_reference.md](api_reference.md) — full public API

## Related documentation

- [Execution domain](../execution/README.md) — shared `src/primitives/execution/` layout
- [Mapping](../mapping/README.md) — KnownDlls bootstrap for clean `ntdll`
- [Primitives domain](../README.md)
