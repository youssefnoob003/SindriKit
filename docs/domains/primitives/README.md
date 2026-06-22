# Primitives Domain

The primitives domain forms the absolute foundation of SindriKit. Everything built on top of the framework (reflective loaders, injectors, evasion modules...) ultimately relies on one of the mechanisms documented here. 

> [!IMPORTANT]
> **Zero Footprint Constraint:** Primitive implementations must avoid introducing new dependencies. For example, module resolution must occur via PEB walking rather than invoking `GetModuleHandle`.

## Table of Contents
- [execution/](execution/)
  Techniques and APIs responsible for executing arbitrary memory (FFI) or transitioning across execution boundaries (Heaven's Gate).
- [memory/](memory/)
  The foundational paradigms for interacting with host process memory (Native vs Win32 allocation).
- [modules/](modules/)
  Techniques for interacting with loaded DLLs and extracting function addresses via PEB traversal.
- [syscalls/](syscalls/)
  The framework's advanced system for bypassing userland EDR hooks via a cascading fallback mechanism.

---

## Global Primitives (`sindri/primitives/ntdll.h`)

SindriKit maintains global state for the `ntdll.dll` base address. This address is used ubiquitously by native components (like the syscall resolver and the native module API) to extract clean stubs or parse export tables without relying on repetitive PEB lookups or the potentially hooked PEB-resident image.

### `snd_set_ntdll`
Sets the globally cached base address of `ntdll.dll`.
- **`ntdll_base`**: Base address to cache. Typically obtained via KnownDlls mapping or PEB walking.

### `snd_get_ntdll`
Retrieves the globally cached base address.
- **Returns:** `PVOID` The base address, or `NULL` if not set.
