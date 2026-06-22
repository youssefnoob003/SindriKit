# Test Payloads (`tests/loader/src/`)

**Location:** `tests/loader/src/`

These C source files are compiled into DLL and EXE payloads that the test runner feeds to the reflective loaders. Each payload is designed to validate a specific capability of the loading pipeline from basic FFI argument passing to TLS callback execution.

All DLL payloads export functions that return sentinel values (`0xFEEDC0DE`, `0x1337C0DE`, etc.) to signal success or specific failure modes. The test runner validates these return values across architecture boundaries.

---

## `test_dll.c`

The primary DLL test payload. Exercises three critical loader capabilities:

### Exports
- **`SayHello(s1, s2, n3)`**: Validates that the FFI bridge correctly passes three arguments. Returns `0xFEEDC0DE` if the arguments exactly match `("bonjour", "hello", 12)`, otherwise `0xDEADBEEF`.
- **`VerifyInit()`**: Validates two things simultaneously:
  1. **DllMain execution**: `DllMain(DLL_PROCESS_ATTACH)` writes `0xA77AC4ED` into a global variable. `VerifyInit` reads it back.
  2. **Relocation correctness**: The global lives in `.data`. If relocations were not applied, the pointer reference will read garbage.
  
  Returns `0xC001D00D` on success.

---

## `test_dll_advanced.c`

Exercises complex loader capabilities: multi-DLL imports, heap allocation, and dynamic loading.

### Exports
- **`AdvancedExport(input_str)`**: Calls `GetSystemInfo` (kernel32), allocates heap memory via `malloc`, copies the input string, dynamically loads `user32.dll` via `LoadLibraryA`, and resolves `MessageBoxA` via `GetProcAddress`. Returns `0x1337C0DE` if all operations succeed with input `"advanced_test"`.
- **`VerifyImports()`**: Stress-tests IAT resolution by calling a diverse set of kernel32 APIs (`GetCurrentProcessId`, `GetCurrentThreadId`, `VirtualAlloc/Free`, `GetSystemTimeAsFileTime`). Also verifies `DllMain` was called exactly once. Returns `0xCA11AB1E` on success.

---

## `test_dll_empty.c`

A completely empty DLL with no export or custom imports beyond what the compiler forces for `DllMain`. Tests that the loader gracefully handles a missing Export Directory without crashing. The test runner expects the loader to report `"Error: Requested export was not found"`.

---

## `test_dll_tls.c`

Registers a TLS callback via the `.CRT$XLB` section using `#pragma section` and `__declspec(allocate)`. The callback writes a sentinel (`0x7153CA11`) into a global variable during `DLL_PROCESS_ATTACH`.

### Exports
- **`VerifyTLS()`**: Reads the sentinel back. Returns `0x71500C01` if the TLS callback fired, `0x715FA110` if it did not.

This validates that the reflective loader correctly parses `IMAGE_TLS_DIRECTORY` and iterates the callback array before invoking `DllMain`.

---

## `test_exe.c`

A basic EXE payload that validates relocations and imports:
1. Checks that a `.data` global (`g_reloc_canary = 0xCAFEBABE`) survived relocation.
2. Calls `GetCurrentProcessId()` through the resolved IAT.

Exits with code `0x7A` (122) on success.

---

## `test_exe_advanced.c`

An advanced EXE payload that validates CRT initialization:
1. Performs `memcpy`, `strlen`, `strcmp` to verify the CRT was bootstrapped.
2. Calls `malloc` and `printf` to exercise heap and stdio.
3. Calls `GetCurrentProcessId` and `GetCurrentThreadId` for multi-API IAT verification.

Exits with code `0x1337` on success. The test runner matches `"Successfully allocated and printed"` in stdout.
