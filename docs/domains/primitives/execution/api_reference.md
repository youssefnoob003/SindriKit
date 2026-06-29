# Execution: API Reference

Public headers: `include/sindri/primitives/ffi.h`, `include/sindri/primitives/heavens_gate.h`.

For the syscall surface (`snd_syscall_resolve`, `snd_syscall_direct_invoke_asm`, `snd_syscall_indirect_invoke_asm`, strategy pipeline), see [syscalls/api_reference.md](../syscalls/api_reference.md).

---

## Dynamic Invocation (`sindri/primitives/ffi.h`)

### `snd_ffi_execute`

Invokes an arbitrary resolved function pointer using a generic `UINT_PTR` argument array. Register placement, stack alignment, and shadow space are handled in MASM (`snd_ffi_bridge_x64` / `snd_ffi_bridge_x86`).

```c
UINT_PTR snd_ffi_execute(PVOID pFunctionAddress, DWORD dwArgCount, const UINT_PTR *pArgs);
```

| Parameter | Type | Description |
|---|---|---|
| `pFunctionAddress` | `PVOID` | Target function; returns `0` immediately if NULL |
| `dwArgCount` | `DWORD` | Number of arguments; may be `0` |
| `pArgs` | `const UINT_PTR *` | Argument array; may be NULL when count is `0` |

**Returns:** `UINT_PTR` — value returned by the target, or `0` on error.

**Error / early-exit conditions:**

| Condition | Result |
|---|---|
| `pFunctionAddress == NULL` | Returns `0` |
| `dwArgCount > 0 && pArgs == NULL` | Returns `0` |
| Unsupported architecture | Returns `0` (compile-time `#error` on non-Windows) |

**Callee-saved registers** (`RBX`, `RBP`, `RDI`, `RSI`, `R12`–`R15` on x64) are preserved by the assembly bridge. The caller must supply arguments compatible with the target's actual signature and calling convention.

**Source:** `src/primitives/execution/ffi/ffi_invoke.c`

---

## Heaven's Gate (`sindri/primitives/heavens_gate.h`)

### `snd_is_wow64`

Detects WoW64 by reading `WOW32Reserved` from the TEB (`__readfsdword(0xC0)` on x86). No Win32 API calls.

```c
BOOL snd_is_wow64(void);
```

| Build | Behavior |
|---|---|
| `_WIN32` (x86) | `TRUE` when `WOW32Reserved != NULL` |
| `_WIN64` | Always `FALSE` |

---

### `snd_hg_execute_64`

Transitions to 64-bit mode via CS selector `0x33` and invokes a 64-bit target. Arguments are copied into a fixed six-slot buffer before entering the ASM bridge (`snd_hg_invoke_x86`).

```c
snd_status_t snd_hg_execute_64(
    UINT64 pFunctionAddress,
    DWORD dwArgCount,
    const UINT64 *pArgs,
    UINT64 *pResult);
```

| Parameter | Type | Description |
|---|---|---|
| `pFunctionAddress` | `UINT64` | 64-bit virtual address of target |
| `dwArgCount` | `DWORD` | Number of 64-bit arguments (max **6**, Microsoft x64 ABI) |
| `pArgs` | `const UINT64 *` | Argument array; required when count > 0 |
| `pResult` | `UINT64 *` | Receives `RAX`; may be NULL (result discarded) |

**Returns:**

| Status | Cause |
|---|---|
| `SND_OK` | Invocation completed |
| `SND_STATUS_INVALID_PARAMETER` | NULL address, count > 6, or count > 0 with NULL `pArgs` |
| `SND_STATUS_UNSUPPORTED` | Native x64 build, non-WoW64 host, or pure 32-bit OS |

**Source:** `src/primitives/execution/heavens_gate/heavens_gate.c`

---

## Internal ASM symbols (not public API)

| Symbol | File | Role |
|---|---|---|
| `snd_ffi_bridge_x64` | `ffi/asm/ffi_x64.asm` | x64 fast-call + shadow space |
| `snd_ffi_bridge_x86` | `ffi/asm/ffi_x86.asm` | Reverse-order stack push, ESP restore |
| `snd_hg_invoke_x86` | `heavens_gate/asm/heavens_gate_x86.asm` | Far return to CS `0x33`, x64 call |
| `snd_syscall_direct_invoke_asm` | `syscalls/asm/invoke_direct_x64.asm` or `invoke_direct_x86.asm` | Direct `syscall` / `sysenter` instruction stub |
| `snd_syscall_indirect_invoke_asm` | `syscalls/asm/invoke_indirect_x64.asm` or `invoke_indirect_x86.asm` | Indirect syscall via NTDLL gadget |

These are linked into `sindri::engine`; callers use the C wrappers above.
