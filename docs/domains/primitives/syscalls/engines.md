# Syscall Resolution Engines

SindriKit ships two built-in SSN resolvers. Both implement `snd_syscall_resolver_t` and plug into the cascading pipeline via `snd_syscall_strategy_set` / `snd_syscall_strategy_add`.

| Resolver | File | Hook resistance |
|---|---|---|
| `snd_syscall_resolve_ssn_scan` | `syscalls_scan.c` | Neighbor scan fallback when stub bytes are patched |
| `snd_syscall_resolve_ssn_sort` | `syscalls_sort.c` | Does not read stub bytes; derives SSN from export order |

---

## Scan resolver (`snd_syscall_resolve_ssn_scan`)

### Algorithm

1. Resolve export address via `snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, func_hash, …)`.
2. Read stub bytes at the export address.
3. Extract SSN from the canonical pattern:
   - **x64:** `4C 8B D1 B8 xx xx` (`mov r10, rcx; mov eax, SSN`)
   - **x86:** `B8 xx xx` (`mov eax, SSN`)
4. If pattern mismatch (typical when inline-hooked), run **neighbor search**.

### Neighbor search

When the target stub is hooked, scans adjacent syscall stubs above and below in memory (step 32 bytes on x64, 16 on x86). For each neighbor with a valid pattern, derives the target SSN by offset index — the same technique family as Halo's Gate.

Caps search at 500 neighbors per direction. Returns `SND_STATUS_SSN_NOT_FOUND` if no valid neighbor is found.

### Output

Populates all `snd_syscall_entry_t` fields: `pAddress`, `dwHash`, `wSystemCall`.

### When to use

- **Primary strategy** — fast path when stubs are unhooked or only partially hooked
- Works well with **clean KnownDlls** `ntdll` where stubs are pristine

---

## Sort resolver (`snd_syscall_resolve_ssn_sort`)

### Algorithm

1. **One-time table build** (`build_syscall_table`):
   - Parse `ntdll` export directory
   - Collect exports prefixed with `Zw`
   - Normalize names to `Nt*` (`ZwCreateFile` → hash of `NtCreateFile`)
   - Store `{hash, address}` pairs
   - **Sort by address ascending**
2. **Lookup:** find matching hash; **array index = SSN**

Windows maps `Zw*` stubs sequentially in memory; sorted export order correlates with SSN assignment.

### Caching

The sorted table is built once per process (`g_table_initialized`). Subsequent resolves are O(n) linear scan over cached entries (n ≤ `SND_MAX_SYSCALLS`).

### Output

Sets `entry_out->wSystemCall` to the table index. Does **not** populate `pAddress` or `dwHash` — sufficient for `snd_syscall_invoke_asm` which only needs the SSN.

### When to use

- **Fallback strategy** when scan fails (heavy hooking, patched stub bytes)
- Immune to inline hooks on individual stubs because SSN is derived from export layout, not stub disassembly

---

## Recommended pipeline

PoCs and production profiles typically register both:

```c
snd_syscall_strategy_set(snd_syscall_resolve_ssn_scan);
snd_syscall_strategy_add(snd_syscall_resolve_ssn_sort);
```

Evaluation order:

1. Try scan (cheap, accurate on clean stubs)
2. On failure, try sort (hook-resistant)

---

## Custom resolvers

Any function matching `snd_syscall_resolver_t` can be registered:

```c
snd_status_t my_resolver(PVOID ntdll, DWORD func_hash, snd_syscall_entry_t *entry_out) {
    // populate entry_out->wSystemCall (required)
    // optionally pAddress, dwHash
    return SND_OK;
}

snd_syscall_strategy_add(my_resolver);
```

Maximum **4** strategies in the chain.

---

## Dependencies

Both resolvers depend on:

- Valid `ntdll_base` passed to `snd_syscall_resolve` (from global `snd_syscall_set_ntdll`)
- PE export parsing (`snd_pe_get_export_address_hash` for scan; manual EAT walk for sort)
- Compile-time function hashes from `sindri_hashes.h` / `config/hashes.ini`

---

## See also

- [Pipeline](pipeline.md) — bootstrap and invocation
- [API reference](api_reference.md)
