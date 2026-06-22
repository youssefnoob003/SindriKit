# PoC: heavens_gate

**Location:** `pocs/heavens_gate/`

This PoC demonstrates SindriKit's capability to transition from a 32-bit WoW64 environment into native 64-bit mode using the `0x33` segment selector (Heaven's Gate).

## What it demonstrates

- Checking whether the current process is running under WoW64.
- Passing a 64-bit function pointer and argument array across the boundary.
- Executing 64-bit shellcode from a 32-bit process and retrieving a 64-bit return value in `RAX`.

## Walkthrough

Because this primitive relies on the WoW64 subsystem, the PoC is only functional when compiled as a 32-bit executable (`x86`) and run on a 64-bit Windows OS.

### Step 1: Validate the Environment

```c
if (!snd_is_wow64()) {
    printf("[-] Not running in WOW64... Heaven's Gate requires WOW64.\n");
    return SND_STATUS_ARCH_MISMATCH;
}
```

The framework parses the PEB to determine if the 32-bit process is being emulated by the 64-bit OS.

### Step 2: Allocate 64-bit Payload

For demonstration purposes, this PoC uses `VirtualAlloc` to allocate a simple 64-bit shellcode block that loads a magic value into `RAX` and returns:

```nasm
mov rax, 0x1122334455667788
ret
```

```c
unsigned char shellcode64[] = {
    0x48, 0xB8, 0x88, 0x77, 0x66,
    0x55, 0x44, 0x33, 0x22, 0x11, 
    0xC3                          
};

PVOID pExec = VirtualAlloc(NULL, sizeof(shellcode64), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
memcpy(pExec, shellcode64, sizeof(shellcode64));
```

### Step 3: Transition and Execute

The PoC invokes the `snd_hg_execute_64` API, which performs the segment selector transition internally, executes the provided address, and passes back the 64-bit return value.

```c
UINT64 result = 0;

// Invoke Heaven's Gate with 0 arguments
snd_status_t status = snd_hg_execute_64((UINT64)(ULONG_PTR)pExec, 0, NULL, &result);

if (status.code == SND_SUCCESS) {
    // result == 0x1122334455667788
}
```

**OpSec impact:** Bypassing WoW64 completely blinds any security products that have only placed userland hooks inside the 32-bit `ntdll.dll`. The execution happens entirely in native 64-bit mode.
