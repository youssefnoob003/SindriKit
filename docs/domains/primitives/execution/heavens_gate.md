# Heaven's Gate

Heaven's Gate transitions a **32-bit WoW64 process** into **native 64-bit mode** by executing through the `0x33` code segment selector, then calling a 64-bit function address with up to six Microsoft x64 ABI arguments.

**Public API:** `include/sindri/primitives/heavens_gate.h`  
**C layer:** `src/primitives/execution/heavens_gate/heavens_gate.c`  
**ASM bridge:** `src/primitives/execution/heavens_gate/asm/heavens_gate_x86.asm` (`snd_hg_invoke_x86`)

---

## The WoW64 boundary

On 64-bit Windows, 32-bit processes run inside the WoW64 subsystem. Userland hooks often target the **32-bit** `ntdll.dll` inside the WoW64 environment. A Heaven's Gate transition executes in **native 64-bit long mode**, bypassing that 32-bit userland layer for the duration of the call.

> [!NOTE]
> Heaven's Gate is only meaningful for **x86 builds on a 64-bit OS under WoW64**. Native x64 processes compile stub implementations that return `SND_STATUS_UNSUPPORTED`. Pure 32-bit Windows has no WoW64 and also fails the check.

---

## WoW64 detection (`snd_is_wow64`)

On x86, the TEB field at offset `0xC0` (`WOW32Reserved`) holds a pointer to the WoW64 transition machinery when running under WoW64:

```c
void *pWow32Reserved = (void *)__readfsdword(0xC0);
return (pWow32Reserved != NULL);
```

No `IsWow64Process` or other Win32 API is used.

---

## Segment selector `0x33`

In 64-bit Windows, CS value **`0x33`** selects 64-bit long mode. The ASM stub performs a far return:

```nasm
push 33h
call next_inst
next_inst:
add dword ptr [esp], 5
retf
```

After `retf`, subsequent instructions execute as x64 until the stub returns through a matching transition back to 32-bit mode.

---

## C wrapper (`snd_hg_execute_64`)

### Argument padding

The ASM bridge expects a **fixed six-element** `UINT64` array. The C layer copies caller arguments into `safe_args[6]` (zero-padded) to prevent out-of-bounds reads in the stub:

```c
UINT64 safe_args[6] = {0};
for (DWORD i = 0; i < dwArgCount; i++) {
    safe_args[i] = pArgs[i];
}
UINT64 result = snd_hg_invoke_x86(pFunctionAddress, safe_args);
```

### Validation

| Check | Status |
|---|---|
| `pFunctionAddress == 0` | `SND_STATUS_INVALID_PARAMETER` |
| `dwArgCount > 6` | `SND_STATUS_INVALID_PARAMETER` (with context string in debug builds) |
| `dwArgCount > 0 && pArgs == NULL` | `SND_STATUS_INVALID_PARAMETER` |
| `!snd_is_wow64()` | `SND_STATUS_UNSUPPORTED` |

### x64 register layout inside the stub

Once in 64-bit mode, the bridge follows the Microsoft x64 convention:

| Slot | Register / stack |
|---|---|
| Arg 1–4 | `RCX`, `RDX`, `R8`, `R9` |
| Arg 5–6 | `[RSP+32]`, `[RSP+40]` above shadow space |
| Return | `RAX` → copied to `*pResult` when non-NULL |

Stack is realigned to 16 bytes and 48 bytes of shadow + spill space is allocated before `CALL RAX`.

---

## Usage example

From `pocs/heavens_gate/main.c`:

```c
#if defined(_WIN64)
    return SND_STATUS_ARCH_MISMATCH;
#elif defined(_WIN32)
    if (!snd_is_wow64()) {
        return SND_STATUS_ARCH_MISMATCH;
    }

    UINT64 result = 0;
    snd_status_t status = snd_hg_execute_64(
        (UINT64)(ULONG_PTR)pShellcode,
        0, NULL, &result);
#endif
```

Build the PoC with an **x86** toolchain target (`cmake -A Win32`). See [examples/heavens_gate.md](../../../examples/heavens_gate.md).

---

## OpSec notes

- The transition itself is an anomalous control-flow pattern (far jump to CS `0x33`) that EDR may flag.
- The PoC allocates shellcode with `VirtualAlloc` for simplicity; production code would use injected memory backends.
- Heaven's Gate does **not** replace syscall bootstrapping for `_sys` backends — it solves a different problem (WoW64 bitness), not SSN resolution.

**Research:** [wow64pp](https://github.com/JustasMasiulis/wow64pp), [WoW64 internals](https://wbenny.github.io/2018/11/04/wow64-internals.html)
