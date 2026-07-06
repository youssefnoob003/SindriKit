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
| `pSyscallAddr` | `PVOID` | Gadget address for indirect invocation (populated by gadget finder) |
| `pSpoofAddr` | `PVOID` | Gadget address for spoofed invocation (populated by spoof finder) |
| `dwSpoofFrameSize` | `DWORD` | Frame size of the spoofed function (populated by spoof finder) |

> [!NOTE]
> The sort resolver sets `wSystemCall` only. `_sys` backends use `entry.wSystemCall` for invocation.

### `snd_syscall_args_t`

Arguments for the syscall invoker (`snd_syscall_direct_invoke_asm` or `snd_syscall_indirect_invoke_asm`). On x64, `arg1` is moved to `R10` per the syscall convention; `arg2`–`arg4` use `RDX`/`R8`/`R9`; `arg5`–`arg11` are stack arguments.

| Field | Type | Description |
|---|---|---|
| `ssn` | `WORD` | SSN placed in `EAX`/`RAX` |
| `arg1`–`arg11` | `PVOID` | Syscall arguments |
| `sys_addr` | `PVOID` | Gadget address for indirect invocation; ignored by direct stub |
| `spoof_addr` | `PVOID` | Gadget address for spoofed invocation; ignored by other stubs |
| `spoof_frame_size` | `DWORD` | Frame size of the spoofed function; ignored by other stubs |

> [!NOTE]
> `sys_addr` is placed at the end of the struct to preserve memory offsets for args 1-11, ensuring backward compatibility with existing ASM stubs.

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

### `snd_syscall_set_resolver`

Sets the **primary** strategy and **resets** the chain to a single entry.

```c
void snd_syscall_set_resolver(snd_syscall_resolver_t resolver);
```

**Returns:** `void`

---

### `snd_syscall_add_resolver`

Appends a fallback strategy. Evaluated in registration order after the primary.

```c
snd_status_t snd_syscall_add_resolver(snd_syscall_resolver_t resolver);
```

**Returns:** `SND_OK`, `SND_STATUS_INVALID_PARAMETER`, `SND_STATUS_PIPELINE_EXHAUSTED` (chain full)

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

### `snd_syscall_set_invoker`

Sets the global syscall invoker function pointer.

```c
void snd_syscall_set_invoker(snd_syscall_invoker_t invoker);
```

Built-in options: `snd_syscall_direct_invoke_asm`, `snd_syscall_indirect_invoke_asm`.

**Source:** `src/primitives/execution/syscalls/syscalls.c`

---

### `snd_syscall_set_gadget_finder`

Sets the global gadget finder function pointer. Only required when using indirect invocation.

```c
void snd_syscall_set_gadget_finder(snd_syscall_gadget_finder_t finder);
```

Built-in: `snd_syscall_find_gadget_scan`.

**Source:** `src/primitives/execution/syscalls/syscalls.c`

---

### `snd_syscall_set_spoof_finder`

Sets the global spoof finder function pointer. Only required when using spoofed invocation.

```c
void snd_syscall_set_spoof_finder(snd_syscall_spoof_finder_t finder);
```

Built-in: `snd_syscall_find_spoof_scan`.

**Source:** `src/primitives/execution/syscalls/syscalls.c`

---

## Resolution & Execution

### `snd_syscall_resolve`

Walks the configured strategy chain; returns on first success.

```c
snd_status_t snd_syscall_resolve(DWORD func_hash, snd_syscall_entry_t *entry_out);
```

**Returns:** `SND_OK`, `SND_STATUS_NOT_INITIALIZED` (no ntdll base or empty chain), `SND_STATUS_SSN_NOT_FOUND` (all strategies failed), `SND_STATUS_INVALID_PARAMETER`

---

### `snd_syscall_direct_invoke_asm`

Architecture-specific ASM stub (`invoke_direct_x64.asm` / `invoke_direct_x86.asm`). Executes the `syscall` / `sysenter` / `int 2Eh` instruction inline.

```c
extern NTSTATUS snd_syscall_direct_invoke_asm(snd_syscall_args_t *args);
```

**Returns:** raw `NTSTATUS` from the kernel

---

### `snd_syscall_indirect_invoke_asm`

Architecture-specific ASM stub (`invoke_indirect_x64.asm` / `invoke_indirect_x86.asm`). Jumps to a legitimate `syscall; ret` gadget inside NTDLL. Requires `sys_addr` to be populated (via `snd_syscall_find_gadget_scan` or a custom gadget finder).

```c
extern NTSTATUS snd_syscall_indirect_invoke_asm(snd_syscall_args_t *args);
```

**Returns:** raw `NTSTATUS`, or `STATUS_INVALID_PARAMETER` (`0xC000000D`) if `sys_addr` is NULL

---

### `snd_syscall_spoofed_invoke_asm`

Architecture-specific ASM stub (`invoke_spoofed_x64.asm` / `invoke_spoofed_x86.asm`). Jumps to a legitimate `syscall; ret` gadget inside NTDLL and spoofs the return address using a JMP-Trampoline inside a Fat Frame. Requires both `sys_addr` (via `snd_syscall_find_gadget_scan`) and `spoof_addr` / `spoof_frame_size` (via `snd_syscall_find_spoof_scan`).

```c
extern NTSTATUS snd_syscall_spoofed_invoke_asm(snd_syscall_args_t *args);
```

**Returns:** raw `NTSTATUS`, or `STATUS_INVALID_PARAMETER` (`0xC000000D`) if any required address is NULL

---

## Compile-Time Defaults (`SND_USE_DEFAULTS`)

| Macro | Default value (when enabled) |
|---|---|
| `SND_SYSCALL_INVOKER_DEFAULT` | `snd_syscall_indirect_invoke_asm` |
| `SND_SYSCALL_GADGET_FINDER_DEFAULT` | `snd_syscall_find_gadget_scan` |
| `SND_SYSCALL_RESOLVER_DEFAULT` | `snd_syscall_resolve_ssn_scan` |

When `SND_USE_DEFAULTS` is disabled (default), all three macros expand to `NULL`.

> [!TIP]
> **OpSec Rationale:** Why use a compile-time macro instead of just initializing the variables to default pointers in `syscalls.c`? 
> If the variables were unconditionally initialized with pointers to `snd_syscall_indirect_invoke_asm` and `snd_syscall_find_gadget_scan`, the C linker would be forced to pull those entire functions (including the scanner logic and ASM stubs) into the final compiled binary, even if the user explicitly chose to use direct syscalls or no syscalls at all. 
> By using `SND_USE_DEFAULTS`, we ensure the default dependency graph is completely severed when disabled, keeping the payload footprint as lean and evasive as possible.

---

## Typical bootstrap (PoC pattern)

```c
PVOID ntdll = NULL;
snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);
snd_syscall_set_ntdll(ntdll);
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);

snd_syscall_entry_t entry = {0};
snd_status_t status = snd_syscall_resolve(SND_HASH_NTALLOCATEVIRTUALMEMORY, &entry);
```

---

## Related documentation

- [Pipeline lifecycle](pipeline.md)
- [Memory `_sys` backend](../memory/techniques.md)
- [Mapping `_sys` backend](../mapping/techniques.md)
