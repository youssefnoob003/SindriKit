# Internal Windows Boundaries

SindriKit supports two kinds of Windows implementation in the same static library:

- **SDK-backed implementations** use the real Win32 declarations and APIs supplied by the Windows SDK.
- **Native/syscall implementations** use project-owned ABI declarations and resolve NT exports or syscall numbers without importing the SDK into those translation units.

This separation keeps the engine suitable for both conventional Win32 builds and reduced-import native/syscall profiles.

## Build contract

SDK-backed source files opt in with:

```text
SND_USE_WINDOWS_SDK=1
```

The CMake target applies this definition only to files that directly use Win32 APIs:

| Source group | Examples |
|---|---|
| File loading | `src/primitives/files/win.c` |
| Win32 memory | `src/primitives/memory/win.c` |
| Win32 mapping | `src/primitives/mapping/win.c` |
| Win32 modules | `src/primitives/modules/win.c` |
| Win32 process | `src/primitives/process/win.c` |
| Win32 thread | `src/primitives/thread/win.c` |
| Execution / Syscalls | `src/primitives/execution/syscalls/finders/spoof.c` |
| Object Manager | `src/primitives/object_manager/knowndlls.c` |

The normal unified PoC opts into SDK mode because it includes `<windows.h>` and exposes the Win32 backend to command-line users. When `SND_CRTLESS=ON`, CMake keeps the shared command sources, uses the PEB-based frontend, and switches the runtime helpers, file capability, and backend defaults to SDK-free implementations. `pocs/src/frontend_crtless.c` obtains arguments from the PEB and enters the same command dispatcher through a direct entrypoint.

## Header layers

Three folders, three jobs:

```text
internal/windows/          shared Windows ABI and on-disk formats (not a callable API)
    types.h                HANDLE/DWORD, calling conventions, SDK selection
    constants.h            PAGE_*, MEM_*, GENERIC_READ, DLL_PROCESS_* (Win32 and NT)
    image.h                IMAGE structures shared by PE and COFF
    pe.h                   PE-specific headers, directories, relocations
    coff.h                 COFF symbols and object relocations

internal/win32/            Win32 callable API
    api.h                  CreateFile, HeapAlloc, ... (SDK-free declarations)
    constants.h            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, HEAP_ZERO_MEMORY
    (umbrella win32.h)

internal/nt/               native NT ABI
    base.h                 NTSTATUS helpers, Unicode strings, object attributes
    file.h                 I/O status and file-information layouts
    process.h              process/thread access, NtCreateUserProcess structures
    peb.h                  PEB and loader layouts
    api.h                  Nt*/Ldr* function-pointer declarations
    (umbrella nt.h)
```

`windows/` is the substrate both APIs share. `win32/` and `nt/` are the two callable surfaces. PE/COFF layouts are file formats, so they live with the ABI under `windows/`, not under `win32/`. Virtual-memory and access-mask flags are the same values for `VirtualAlloc` and `NtAllocateVirtualMemory`, so they live in `windows/constants.h`; NT and syscall backends must not include `win32/` to obtain them. `win32/constants.h` includes that header and adds CreateFile/Heap flags that have no NT equivalent (`OPEN_EXISTING` is not `FILE_OPEN`).

`win32.h` and `nt.h` are umbrellas for their own layer only. Code that needs only a small header should include that header directly: parsers take `windows/pe.h` or `windows/coff.h`; NT backends take `nt/api.h` plus `windows/constants.h` for `SND_PAGE_*` / `SND_MEM_*`; type-only public headers take `windows/types.h`.

## Rules for contributors

1. Keep Windows SDK inclusion explicit. Do not add unconditional `<windows.h>` includes to shared headers.
2. Keep project-owned constants prefixed with `SND_`.
3. Keep PE/COFF file-format structures independent from NT process structures.
4. Use `windows/image.h` for structures shared by PE and COFF; do not make `coff.h` depend on the complete PE header.
5. SDK-backed files must call the actual Windows functions. The native declarations are only for SDK-free compilation and link to the same OS APIs.
6. Prefer narrow internal includes over umbrella headers when adding new code.
7. Do not include `win32.h` just to get `HANDLE`, `IMAGE_*` types, or `SND_PAGE_*` / `SND_MEM_*` flags.

## See also

- [Building](../getting_started/building.md)
- [Common infrastructure](../common/infrastructure.md)
- [Dependency injection](dependency_injection.md)
