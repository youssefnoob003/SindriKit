# `unified hg` — Heaven's Gate

**Implementation:** `pocs/src/cmd_hg.c`
**Syntax:** `unified hg` (no options)

Executes 64-bit shellcode from a 32-bit WoW64 process by switching to the native x64 code segment (`0x33`). The demo shellcode returns a magic value in `RAX`, which the command prints and verifies.

## What it demonstrates

- WoW64 environment detection (`snd_is_wow64`)
- 64-bit call bridging from a 32-bit host (`snd_hg_execute_64` + the MASM x86 stub)
- Backend-neutral executable allocation through the injected memory API

## Availability

| Build | Behavior |
|---|---|
| x86 (`_WIN32`) | `hg` is compiled into the dispatcher. At runtime it requires an actual WoW64 host. |
| x64 (`_WIN64`) | The command is not compiled in (`main.c` gates it with `#if defined(_WIN32) && !defined(_WIN64)`); `snd_hg_execute_64` itself returns `SND_STATUS_ARCH_MISMATCH`. |

If `snd_is_wow64()` returns FALSE, the command prints an error and returns `SND_STATUS_ARCH_MISMATCH`.

## Walkthrough

### 1. Validate the environment

```c
if (!snd_is_wow64()) {
    return SND_STATUS_ARCH_MISMATCH;
}
```

### 2. Prepare 64-bit shellcode

The demo payload loads a magic value and returns:

```nasm
mov rax, 0x1122334455667788
ret
```

```c
unsigned char shellcode64[] = {
    0x48, 0xB8, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
    0xC3
};

const snd_memory_api_t *memory = &snd_mem_win;  // snd_mem_nt in CRT-less builds
memory->alloc(NULL, sizeof(shellcode64),
              SND_MEM_COMMIT | SND_MEM_RESERVE, SND_PAGE_EXECUTE_READWRITE, &pExec);
snd_memcpy(pExec, shellcode64, sizeof(shellcode64));
```

### 3. Execute through the bridge

```c
ULONGLONG    result = 0;
snd_status_t st = snd_hg_execute_64((ULONGLONG)(ULONG_PTR)pExec, 0, NULL, &result);
// result == 0x1122334455667788 on success
```

`snd_hg_execute_64` accepts up to `SND_HG_MAX_ARGS` (6) arguments, copies them into a fixed array, and rejects a NULL argument vector with a non-zero count. The return value is reported through `pResult` when provided.

### 4. Cleanup

```c
memory->free(pExec, 0, SND_MEM_RELEASE);
```

The command propagates the execution status: success returns `SND_SUCCESS`, failure returns the failing `snd_status_t.code`.

## Building

An **x86** target is required. `build.bat pocs` builds both architectures, so the x86 binary lands in `build32/`:

```bat
build.bat pocs
:: → build32/pocs/Release/unified.exe
```

Equivalent raw CMake (x86 only):

```bash
cmake -B build -A Win32 -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
# → build/pocs/Release/unified.exe
```

CRT-less builds keep the same command (`build.bat pocs crtless`); the demo allocation switches to `snd_mem_nt` because the Win32 backend is unavailable.

## OpSec impact

The transition bypasses 32-bit userland hooks for the 64-bit code itself. The demo uses `PAGE_EXECUTE_READWRITE` for simplicity, which is its most visible property — production use should prefer staging the buffer as RW and flipping to RX before execution.

## See also

- [Heaven's Gate API](../primitives/execution/heavens_gate.md)
- [Execution domain](../primitives/execution/README.md)
- [cli.md](cli.md) — command dispatch and backends
