# Execution: API Reference

This page documents the public API surface for execution boundary transitions and dynamic invocation.

---

## Dynamic Invocation (`sindri/primitives/ffi.h`)

### `snd_execute_dynamic`

Invokes an arbitrary resolved function pointer at runtime using a generic argument array. Handles all calling convention requirements (register placement, stack alignment, shadow space) natively in MASM.

| Parameter | Type | Description |
|---|---|---|
| `pFunctionAddress` | `PVOID` | Target function pointer; returns 0 immediately if NULL |
| `dwArgCount` | `DWORD` | Number of arguments in `pArgs`; may be 0 |
| `pArgs` | `const UINT_PTR*` | Array of `dwArgCount` arguments; may be NULL if count is 0 |

**Returns:** `UINT_PTR` — the return value of the invoked function, or 0 on error.

---

## Heaven's Gate (`sindri/primitives/heavens_gate.h`)

### `snd_is_wow64`

Checks whether the current 32-bit process is executing under WoW64 by reading the `WOW32Reserved` field directly from the Thread Environment Block (TEB). No Win32 API calls are made.

**Returns:** `BOOL` — `TRUE` if running under WoW64, `FALSE` otherwise.

---

### `snd_hg_execute_64`

Transitions to 64-bit mode via the `0x33` segment selector and invokes the specified 64-bit function.

| Parameter | Type | Description |
|---|---|---|
| `pFunctionAddress` | `UINT64` | 64-bit virtual address of the target function |
| `dwArgCount` | `DWORD` | Number of 64-bit arguments (Max: 6) |
| `pArgs` | `const UINT64*` | Array of 64-bit arguments |
| `pResult` | `UINT64*` | Receives the 64-bit return value (`RAX`) |

**Returns:** `snd_status_t` — `SND_OK` on success, or an error if the process is not running under WOW64.
