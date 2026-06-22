# Syscalls: API Reference

This page documents the public API surface for System Service Number (SSN) resolution and direct syscall execution. All definitions reside in `include/sindri/primitives/syscalls.h`.

---

## Core Data Structures

### `snd_syscall_entry_t`
Structure holding the successfully resolved context of a target syscall.

| Field | Type | Description |
|---|---|---|
| `pAddress` | `PVOID` | Pointer to the original stub address in memory |
| `dwHash` | `DWORD` | The compile-time hash of the target NT function |
| `wSystemCall` | `WORD` | The resolved System Service Number (SSN) |

### `snd_syscall_args_t`
Arguments structure passed to the generic ASM syscall invoker.

| Field | Type | Description |
|---|---|---|
| `ssn` | `WORD` | System Service Number to place in the `EAX`/`RAX` register |
| `arg1`–`arg10` | `PVOID` | Up to 10 arguments forwarded to the kernel |

---

## Resolvers & Pipeline Configuration

### `snd_syscall_resolver_t`
Function pointer signature for SSN resolution strategies. Any custom gate can be plugged into the framework if it matches this signature.

```c
typedef snd_status_t (*snd_syscall_resolver_t)(PVOID ntdll_base, DWORD func_hash, snd_syscall_entry_t *entry_out);
```

#### Built-in Resolvers
- `snd_hell_extract_syscall`: Standard Hell's Gate
- `snd_halo_extract_syscall`: Standard Halo's Gate (`0xE9` hook evasion)
- `snd_tartarus_extract_syscall`: Tartarus' Gate (`0xEB` hook evasion)
- `snd_veles_extract_syscall`: VelesReek (`syscall` instruction anchor)

---

### `snd_set_syscall_strategy`
Sets the primary resolution strategy and **resets** the entire fallback chain to a single entry.

| Parameter | Type | Description |
|---|---|---|
| `resolver` | `snd_syscall_resolver_t` | The resolver function to use as the primary strategy |

---

### `snd_add_syscall_strategy`
Appends a fallback strategy to the resolution pipeline. The pipeline is evaluated in registration order. Maximum depth: 4 strategies.

| Parameter | Type | Description |
|---|---|---|
| `resolver` | `snd_syscall_resolver_t` | The fallback resolver function to append |

**Returns:** `snd_status_t` — `SND_OK` on success, `SND_STATUS_SSN_BUFFER_TOO_SMALL` if the chain is full.

---

### NTDLL Base Address

> [!NOTE]
> The `ntdll.dll` base address is managed globally via `snd_set_ntdll` and `snd_get_ntdll` from `sindri/primitives/ntdll.h`. You MUST set the global base before attempting to resolve syscalls.

---

## Execution

### `snd_resolve_syscall`
Attempts to resolve a System Service Number (SSN) by running the configured strategy pipeline in order. Returns on the first successful result.

| Parameter | Type | Description |
|---|---|---|
| `func_hash` | `DWORD` | Compile-time hash of the target NT function name |
| `entry_out` | `snd_syscall_entry_t*` | Output structure populated with the resolved SSN and stub address |

**Returns:** `snd_status_t` — `SND_OK` on success, `SND_STATUS_SSN_NOT_FOUND` if all strategies fail, `SND_STATUS_NOT_INITIALIZED` if `snd_set_ntdll` was not called.

---

### `snd_invoke_syscall_asm`
ASM stub that performs the actual `syscall` instruction. Architecture-specific implementations exist for x64 and x86.

| Parameter | Type | Description |
|---|---|---|
| `args` | `snd_syscall_args_t*` | Pointer to a populated arguments structure |

**Returns:** `NTSTATUS` — the raw kernel return value.
