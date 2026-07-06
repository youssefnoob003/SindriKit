# Syscall Resolution Engines

SindriKit ships two built-in SSN resolvers. Both implement `snd_syscall_resolver_t` and plug into the cascading pipeline via `snd_syscall_set_resolver` / `snd_syscall_add_resolver`.

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

Sets `entry_out->wSystemCall` to the table index. Does **not** populate `pAddress` or `dwHash` — sufficient for the invoker which only needs the SSN.

### When to use

- **Fallback strategy** when scan fails (heavy hooking, patched stub bytes)
- Immune to inline hooks on individual stubs because SSN is derived from export layout, not stub disassembly

---

## Recommended pipeline

PoCs and production profiles typically register both:

```c
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);
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

snd_syscall_add_resolver(my_resolver);
```

Maximum **4** strategies in the chain.

---

## Gadget finder (`snd_syscall_find_gadget_scan`)

**Source:** `gadget_scan.c`

Resolves the target function address from the natively loaded NTDLL (via PEB walk, not the user-supplied NTDLL image) and scans for a transition gadget.

### Algorithm

1. Get the natively loaded `ntdll.dll` base via `snd_peb_get_module_base_hash`.
2. Resolve the export address of the target function using its hash.
3. Scan forward up to 32 bytes for the transition signature:
   - **x64:** `syscall; ret` (`0F 05 C3`)
   - **x86:** Entry point + 5 bytes (skips `mov eax, SSN` and enters the OS-specific transition)

### Output

Populates `entry->pSyscallAddr` with the gadget address.

### When to use

Required when using `snd_syscall_indirect_invoke_asm`. The gadget finder runs automatically during `snd_syscall_resolve` if `g_syscall_gadget_finder` is set.

> [!IMPORTANT]
> The gadget finder uses the **natively loaded** NTDLL from the process PEB, not the user-supplied NTDLL image. This ensures the gadget points to executable memory regardless of how the SSN source was obtained (disk load, KnownDlls map, etc.).

---

## Spoof finder (`snd_syscall_find_spoof_scan`)

**Source:** `spoof_scan.c`

Locates a trampoline gadget (`RET`) within a legitimate function in `kernel32.dll` to spoof the call stack return address.

### Algorithm (x64)

To prevent frame desynchronization during stack unwinding (where the EDR's `RtlVirtualUnwind` crashes trying to parse our spoofed return address), the spoof finder dynamically hunts for a **"Fat Frame"**.

1. Resolve the natively loaded `kernel32.dll` via PEB.
2. Manually parse the **Exception Directory** (`.pdata`) of `kernel32.dll` (avoiding native APIs like `RtlLookupFunctionEntry` for stealth).
3. Iterate through `RUNTIME_FUNCTION` entries and parse their `UNWIND_INFO` structures to calculate the total stack space allocated by each function.
4. Find a function that allocates at least **120 bytes** (a "Fat Frame"). This ensures the legitimate function's shadow space is large enough to encapsulate our spoofed JMP-trampoline and its arguments.
5. Scan the body of the Fat Frame function for a `0xC3` (`RET`) instruction.
6. Populate `entry->pSpoofAddr` with the gadget address and `entry->dwSpoofFrameSize` with the required stack offset.

### Algorithm (x86)

x86 does not use `.pdata` for exception handling. The scanner falls back to resolving `BaseThreadInitThunk` and scanning forward/backward for a simple `RET` gadget.

### When to use

Required when using `snd_syscall_spoofed_invoke_asm`. The spoof finder runs automatically during `snd_syscall_resolve` if `g_syscall_spoof_finder` is set.

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
