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
