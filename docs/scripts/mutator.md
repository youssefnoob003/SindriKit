# Mutation Engine (`SND_MORPH`)

The SindriKit Mutation Engine is a pre-build pipeline that introduces structural and instruction-level polymorphism into the compiled binary. When enabled, it dynamically alters the C source code, assembly stubs, and internal data structures before they are compiled, generating a unique binary signature for every build without altering the semantic behavior of the framework.

To enable the mutator, pass the `SND_MORPH` flag to CMake:
```bash
cmake -B build64 -A x64 -DSND_MORPH=ON
```

## How it works

When `SND_MORPH=ON` is provided, CMake intercepts the build process and executes `scripts/mutator/morph.py`. This orchestrator creates a temporary `morphed/` directory inside your build folder. It copies the `src/` and `include/` files into this directory and sequentially applies a series of mutation passes before compilation begins.

The compilation targets (like `sindri_engine`) are then transparently redirected to compile from the `morphed/` directory instead of the root directory.

## Mutation Passes

### 1. C Source Mutator (`junk_c.py`)
This pass injects volatile-backed opaque predicates directly into the execution flow of C functions. 

**Features:**
- **Opaque Predicates:** Generates logic blocks using `volatile` variables and random entropy that evaluate to predictable outcomes but appear highly dynamic to static analysis.
- **Multiple Shapes:** Cycles through various logical structures (simple `if`, opaque `while`, dummy `switch`, `do-while`) to break basic structural signatures.
- **Control Flow Awareness:** The parser tracks C scope braces (`{`, `}`) and intelligently buffers statements to avoid injecting junk code immediately after control-flow exit statements (like `return`, `break`, `continue`, or `goto`). This guarantees that all injected junk code is mathematically reachable, preventing `C4702` (unreachable code) compiler errors.
- **Dead Function Generation & Call Graph Splitting:** Generates completely random, non-existent `static` C functions at the global scope of the file. These functions contain non-signatured math loops. The opaque predicates are then configured to randomly call these dead functions from within their unreachable branches. Because the branch conditions rely on `volatile` variables, the compiler cannot prove they are dead code and is forced to compile and link them, drastically expanding the function call graph and wasting reverse-engineering time.

### 2. Assembly Mutator (`masm_mutate.py`)
This pass adds instruction-level polymorphism to the hand-written MASM stubs (e.g., FFI, Syscall Invokers, Heaven's Gate).

**Features:**
- **Functional NOPs:** Injects single-byte or multi-byte `nop` equivalents (e.g., `xchg eax, eax`) randomly between instructions.
- **Junk Math:** Injects mathematically neutral operations (`add reg, 0`, `xor reg, 0`, `and reg, reg`) into the instruction stream.
- **Architecture Awareness:** Dynamically identifies whether the target file is x86 or x64. It strictly injects 32-bit registers (`eax`, `ecx`, `edx`) for x86 files and 64-bit registers (`rax`, `r8`, `r9`) for x64 files, preventing `A2006` undefined register errors.
- **ABI Safety:** It avoids injecting math instructions that clobber `EFLAGS` immediately following `test` or `cmp` operations. It also explicitly avoids using or swapping registers that are rigidly defined by the Windows x64 syscall ABI (such as `r10` and `r11`).

### 3. Structural Polymorphism (`struct_shuffle.py`)
This pass scrambles the memory layout of internal framework structures. By changing the offsets of variables dynamically, static signature scans mapping specific struct layouts will fail.

**Features:**
- **Opt-in Macros:** Structs are flagged for shuffling by wrapping them in `SND_SHUFFLE_START` and `SND_SHUFFLE_END` macros (defined in `include/sindri/common/macros.h`). These evaluate to nothing during compilation but act as sentinels for the Python parser.
- **Nested Block Safety:** The parser actively tracks C brace depth, ensuring that nested structures or anonymous unions (e.g., `union { PIMAGE_NT_HEADERS32 nt32; PIMAGE_NT_HEADERS64 nt64; }`) are treated as atomic fields and are shuffled intact without destroying their internal composition.

## Related documentation

- [Config: CMake build system](../getting_started/README.md)
- [Technique: Syscall Invocation](../domains/primitives/syscalls/README.md)
