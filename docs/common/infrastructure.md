# Common Infrastructure

The `include/sindri/common/` headers provide the shared utilities that every domain in the framework depends on. They solve three fundamental problems: eliminating the C Runtime dependency, tracking buffer bounds to prevent memory corruption, and removing plaintext API strings from the compiled binary.

---

## CRT Independence

SindriKit is designed to compile under `/NODEFAULTLIB` (MSVC), completely stripping the Microsoft C Runtime from the final binary. This is a hard requirement for position-independent shellcode and for minimizing the import table footprint of implants.

The standard C library functions that the framework requires are reimplemented as compiler-intrinsic-safe inline functions inside `include/sindri/common/helpers.h`:

| CRT Function | SindriKit Replacement | Notes |
|---|---|---|
| `memset` | `snd_memzero` | Uses `volatile BYTE*` to prevent compiler optimization from eliding the zeroing loop |
| `memcpy` | `snd_memcpy` | Byte-by-byte copy with no alignment assumptions |
| `strnlen` | `snd_strnlen` | Bounded length evaluation |
| `strncpy` / `strncpy_s` | `snd_strncpy` | Truncation-safe; always null-terminates |
| `strcat` / `strcat_s` | `snd_strncat` | Bounded concatenation with null-termination guarantee |
| `strchr` | `snd_strnchr` | Bounded character search |
| `strcmp` / `strncmp` | `snd_strncmp` | Bounded comparison returning standard `<0 / 0 / >0` semantics |

Additionally, `src/common/crt_manifest.c` provides the compiler-required `memcpy` and `memset` symbols that MSVC's code generation emits implicitly (e.g., for struct copies and zero-initialization). Without these symbols, linking under `/NODEFAULTLIB` produces unresolved external errors even if the source code never explicitly calls `memcpy`.

> [!CAUTION]
> Enabling `SND_DEBUG=1` or `SND_USE_PRINTF=1` pulls in `<stdio.h>` and `<stdarg.h>`, which reintroduce CRT dependencies. These flags **must** be disabled (`SND_DEBUG=0`) for any `/NODEFAULTLIB` build.

---

## Buffer Bounds Tracking

SindriKit wraps all raw memory pointers in a `snd_buffer_t` structure that pairs the data pointer with a tracked size. Every access into a buffer — whether parsing PE headers, walking export tables, or applying relocations — is routed through bounds-checking functions that validate `(offset, size)` against the tracked total before dereferencing.

### `snd_buffer_t`

```c
struct snd_buffer_s {
    LPVOID     data;
    SIZE_T     size;
    snd_free_cb free_routine;  // optional cleanup callback
};
```

The `free_routine` callback supports polymorphic deallocation: the same `snd_buffer_free` function correctly handles buffers backed by `HeapAlloc`, `VirtualAlloc`, or `MapViewOfFile` by dispatching through the stored callback.

### Bounds Checking Pipeline

All bounds validation is implemented as force-inlined functions to eliminate call overhead:

1. `snd_bounds_check(total_size, offset, size)` — pure arithmetic validation.
2. `snd_buffer_bounds_check(buf, offset, size)` — validates against a tracked `snd_buffer_t`.
3. `snd_ptr_bounds_check(base, total_size, ptr, size)` — validates an arbitrary pointer against a known base region.

These functions catch both overflow and underflow conditions. The PE parser's `snd_pe_rva_to_ptr` is the primary consumer, rejecting any RVA that would resolve outside the tracked image.

---

## API Hashing

To eliminate plaintext API strings (like `NtAllocateVirtualMemory` or `kernel32.dll`) from the binary's `.rdata` section, SindriKit pre-computes hashes of all required API names at configure time. The CMake build system runs `scripts/generate_hashes.py` against `config/hashes.ini`, which lists every module and function the framework resolves.

### Hash Variants

Three runtime hashing functions are provided to match the contexts in which strings are encountered:

| Function | Input | Casing | Use Case |
|---|---|---|---|
| `snd_hash` | `const char*` | Case-sensitive | Export names from the PE Export Directory (e.g., `NtAllocateVirtualMemory`) |
| `snd_hash_lower` | `const char*` | Lowercased | DLL names from the PE Import Directory (e.g., `KERNEL32.dll` → lowered) |
| `snd_hash_wide_lower` | `const wchar_t*` | Lowercased | Module names from the PEB `BaseDllName` (UTF-16) |

The hashing algorithm itself is swappable at configure time via the `SND_HASH_ALGO` CMake variable (e.g., `DJB2`, `FNV1A`). The Python script also generates a randomized compile-time seed per build run, ensuring that hash values are unique across builds and cannot be signature-matched.

---

## Debug Output System

SindriKit implements a two-tier debug output system controlled by the `SND_DEBUG` and `SND_USE_PRINTF` preprocessor macros:

| `SND_DEBUG` | `SND_USE_PRINTF` | Output Destination | CRT Required |
|---|---|---|---|
| `0` | N/A | All macros compile to no-ops; zero strings emitted | No |
| `1` | `1` | `stdout` / `stderr` via `fprintf` | Yes |
| `1` | `0` | Windows kernel debugger via `OutputDebugStringA` | Partial (`vsnprintf`) |

The two primary macros are:
- `SND_DEBUG_PRINT(fmt, ...)` — outputs to the default destination.
- `SND_FDEBUG_PRINT(stream, fmt, ...)` — outputs to a specific file stream (only meaningful when `SND_USE_PRINTF=1`).

When `SND_DEBUG=0`, both macros expand to empty `do { (void)0; } while(0)` blocks. The compiler strips all format string literals and argument evaluations from the binary entirely.

---

## Disk I/O

The `snd_buffer_load_from_disk` utility reads an entire file into a heap-allocated `snd_buffer_t`. It uses Win32 `CreateFileA` / `ReadFile` internally and sets the buffer's `free_routine` to `snd_buffer_free_heap` for automatic cleanup.

This function is used exclusively by the PoC executables to load raw PE payloads from disk. In a production implant, the payload would typically arrive over a network channel and be written directly into a pre-allocated buffer, bypassing disk I/O entirely.
