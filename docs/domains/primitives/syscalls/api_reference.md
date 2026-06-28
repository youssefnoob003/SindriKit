# Syscalls: API Reference

Public API in `include/sindri/primitives/syscalls.h`. Implementation: `src/primitives/execution/syscalls/`.

---

## Constants

| Macro | Value | Description |
|---|---|---|
| `SND_MAX_SYSCALLS` | `500` | Max cached exports in sort resolver |
| `SND_MAX_SYS_NAME_LEN` | `256` | Max normalized export name length |

Internal pipeline depth: **4 strategies** (`SND_MAX_INTERNAL_STRATEGIES` in `syscalls.c`).

---

## Core Data Structures

### `snd_syscall_entry_t`

| Field | Type | Description |
|---|---|---|
| `pAddress` | `PVOID` | Stub address (populated by scan resolver) |
| `dwHash` | `DWORD` | Target function hash |
| `wSystemCall` | `WORD` | Resolved SSN |

> [!NOTE]
> The sort resolver sets `wSystemCall` only. `_sys` backends use `entry.wSystemCall` for invocation.

### `snd_syscall_args_t`

Arguments for `snd_syscall_invoke_asm`. On x64, `arg1` is moved to `R10` per the syscall convention; `arg2`–`arg4` use `RDX`/`R8`/`R9`; `arg5`–`arg11` are stack arguments.

| Field | Type | Description |
|---|---|---|
| `ssn` | `WORD` | SSN placed in `EAX`/`RAX` |
| `arg1`–`arg11` | `PVOID` | Syscall arguments |

---

## Pipeline Configuration

### `snd_syscall_resolver_t`

```c
typedef snd_status_t (*snd_syscall_resolver_t)(
    PVOID ntdll_base,
    DWORD func_hash,
    snd_syscall_entry_t *entry_out
);
```

### `snd_syscall_strategy_set`

Sets the **primary** strategy and **resets** the chain to a single entry.

```c
void snd_syscall_strategy_set(snd_syscall_resolver_t resolver);
```

**Returns:** `void`

---

### `snd_syscall_strategy_add`

Appends a fallback strategy. Evaluated in registration order after the primary.

```c
snd_status_t snd_syscall_strategy_add(snd_syscall_resolver_t resolver);
```

**Returns:** `SND_OK`, `SND_STATUS_INVALID_PARAMETER`, `SND_STATUS_BUFFER_TOO_SMALL` (chain full)

---

### `snd_syscall_set_ntdll`

Registers the global `ntdll` base used by `snd_syscall_resolve`.

```c
void snd_syscall_set_ntdll(PVOID ntdll_base);
```

No-op if `ntdll_base` is NULL. Typical sources: KnownDlls map, PEB walk, disk buffer.

**Source:** `src/primitives/execution/syscalls/syscalls.c`

---

## Built-in Resolvers

### `snd_syscall_resolve_ssn_scan`

Stub-byte SSN extraction with neighbor fallback. See [engines.md](engines.md).

```c
snd_status_t snd_syscall_resolve_ssn_scan(
    PVOID ntdll_base, DWORD func_hash, snd_syscall_entry_t *entry_out);
```

**Source:** `src/primitives/execution/syscalls/syscalls_scan.c`

---

### `snd_syscall_resolve_ssn_sort`

EAT sort-table SSN derivation. See [engines.md](engines.md).

```c
snd_status_t snd_syscall_resolve_ssn_sort(
    PVOID ntdll_base, DWORD func_hash, snd_syscall_entry_t *entry_out);
```

**Source:** `src/primitives/execution/syscalls/syscalls_sort.c`

---

## Resolution & Execution

### `snd_syscall_resolve`

Walks the configured strategy chain; returns on first success.

```c
snd_status_t snd_syscall_resolve(DWORD func_hash, snd_syscall_entry_t *entry_out);
```

**Returns:** `SND_OK`, `SND_STATUS_NOT_INITIALIZED` (no ntdll base or empty chain), `SND_STATUS_SSN_NOT_FOUND` (all strategies failed), `SND_STATUS_INVALID_PARAMETER`

---

### `snd_syscall_invoke_asm`

Architecture-specific ASM stub (`invoke_x64.asm` / `invoke_x86.asm`).

```c
extern NTSTATUS snd_syscall_invoke_asm(snd_syscall_args_t *args);
```

**Returns:** raw `NTSTATUS` from the kernel

---

## Typical bootstrap (PoC pattern)

```c
PVOID ntdll = NULL;
snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);
snd_syscall_set_ntdll(ntdll);
snd_syscall_strategy_set(snd_syscall_resolve_ssn_scan);
snd_syscall_strategy_add(snd_syscall_resolve_ssn_sort);

snd_syscall_entry_t entry = {0};
snd_status_t status = snd_syscall_resolve(SND_HASH_NTALLOCATEVIRTUALMEMORY, &entry);
```

---

## Related documentation

- [Pipeline lifecycle](pipeline.md)
- [Memory `_sys` backend](../memory/techniques.md)
- [Mapping `_sys` backend](../mapping/techniques.md)
