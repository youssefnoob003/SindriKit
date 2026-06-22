# Dynamic Invocation FFI

The Foreign Function Interface (FFI) primitives provide the capability to invoke an arbitrary, dynamically resolved function pointer at runtime using a generic array of arguments. This is primarily required by reflective loaders to call exports (like `DllMain`) from an in-memory PE image where the exact function signature is completely unknown at compile-time.

## Architecture-Specific Bridges

Calling an arbitrary function pointer in C typically requires casting it to a strictly typed signature. SindriKit implements architecture-specific MASM bridges to bypass this requirement and dynamically manipulate the stack and registers.

### x64 Implementation
The `snd_execute_dynamic` primitive for x64 is implemented as a MASM assembly bridge that strictly adheres to the Microsoft x64 calling convention:
1. It extracts the first four arguments from the provided array and places them into the fast-call registers: `RCX`, `RDX`, `R8`, and `R9`.
2. Any remaining arguments (argument 5 and beyond) are spilled onto the stack above the mandatory 32-byte shadow space.
3. The bridge forcefully ensures the stack pointer (`RSP`) remains 16-byte aligned before issuing the final `CALL` instruction, preventing fatal `EXCEPTION_DATATYPE_MISALIGNMENT` crashes that frequently plague unstable redteam loaders.

### x86 Implementation
The 32-bit x86 implementation operates differently:
1. It iterates through the argument array in reverse order, pushing each argument sequentially onto the stack.
2. It invokes the target pointer.
3. Crucially, the bridge records the stack pointer (`ESP`) before pushing arguments and restores it immediately after the call returns. This provides universal support for both `__cdecl` (caller cleans up) and `__stdcall` (callee cleans up) targets without requiring prior knowledge of the target's exact calling convention.

## Type Punning
Internally, the framework passes function pointers through `UINT_PTR` variables rather than explicit function pointer types to suppress MSVC C4152 compiler warnings, adhering to strict zero-warning compilation policies.
