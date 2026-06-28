# PoC: heavens_gate

**Location:** `pocs/heavens_gate/`

Demonstrates transitioning from a 32-bit WoW64 process into native 64-bit execution using the `0x33` segment selector (Heaven's Gate). Executes 64-bit shellcode and retrieves the 64-bit return value in `RAX`.

## What it demonstrates

- WoW64 environment detection (`snd_is_wow64`)
- 64-bit execution from a 32-bit host via `snd_hg_execute_64`
- Native x64 compile rejection at runtime

## Walkthrough

### 1. Validate environment

Requires **x86 build** on a **64-bit Windows** host under WoW64:

```c
#if defined(_WIN64)
    return SND_STATUS_ARCH_MISMATCH;
#elif defined(_WIN32)
    if (!snd_is_wow64()) {
        return SND_STATUS_ARCH_MISMATCH;
    }
#endif
```

### 2. Allocate 64-bit shellcode

Demo shellcode loads a magic value into `RAX` and returns:

```nasm
mov rax, 0x1122334455667788
ret
```

```c
unsigned char shellcode64[] = {
    0x48, 0xB8, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
    0xC3
};

PVOID pExec = VirtualAlloc(NULL, sizeof(shellcode64),
                             MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
memcpy(pExec, shellcode64, sizeof(shellcode64));
```

> [!NOTE]
> Win32 allocation for demo simplicity. Production use would use `snd_mem_*` backends.

### 3. Execute via Heaven's Gate

```c
UINT64       result = 0;
snd_status_t status = snd_hg_execute_64(
    (UINT64)(ULONG_PTR)pExec,
    0, NULL, &result
);
// result == 0x1122334455667788 on success
```

> [!NOTE]
> The PoC prints failure messages but **`main` returns `SND_SUCCESS` (0)** on the WoW64 path regardless of `snd_hg_execute_64` outcome. Use debug output or patch `main` for status-driven exit codes.

## Building

Requires an **x86** toolchain target:

```bash
cmake -B build -A Win32 -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
```

Output: `build/pocs/heavens_gate/Release/heavens_gate.exe`

Excluded when `SND_CRTLESS=ON` (only `loader_noCRT_nowinapi` builds).

## OpSec impact

Bypasses 32-bit userland hooks during the transition. The PoC uses visible `VirtualAlloc` for the demo shellcode buffer.

## See also

- [Heaven's Gate API](../domains/primitives/execution/heavens_gate.md)
- [Execution domain](../domains/primitives/execution/README.md)
