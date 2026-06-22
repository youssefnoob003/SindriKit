# Evasive Loader (No-CRT / No-WinAPI)

**Path:** `pocs/loader_noCRT_nowinapi`

This Proof-of-Concept demonstrates the ultimate operational profile of SindriKit: a reflective loader completely independent of both high-level Windows APIs and the Microsoft C Runtime (CRT). By combining direct syscalls via Hell's Gate, PEB module walking, and the `/NODEFAULTLIB` compiler flag, this loader achieves a minimal, stealthy footprint ideal for production implants.

## 1. CRT Independence

When compiled in Release mode (`SND_ENABLE_DEBUG=OFF`), this PoC invokes strict CRT-stripping flags:
```cmake
target_link_options(loader_noCRT_nowinapi PRIVATE
    /NODEFAULTLIB
    /ENTRY:main
)
```
Without the CRT, standard functions like `memcpy` or `memset` are unavailable. SindriKit compensates for this by selectively compiling `src/common/crt_manifest.c` in Release builds, providing framework-native intrinsic fallbacks. This ensures the compiled artifact contains no implicit telemetry or standard library dependencies.

## 2. Global `ntdll.dll` Resolution

Because the PoC operates without Win32 APIs (like `GetModuleHandle`), it must resolve its own dependencies manually. It does this by walking the Process Environment Block (PEB) to locate `ntdll.dll` by its compile-time hash.

```c
PVOID ntdll;
status = snd_peb_get_module_base_by_hash(SND_HASH_NTDLL_DLL, &ntdll);
```

Once resolved, it globally registers the `ntdll` base using the modules API, allowing the rest of the framework (like the native memory allocator or module loader) to utilize it implicitly.

```c
snd_set_ntdll(ntdll);
```

## 3. Syscall Strategy

The PoC activates the `snd_hell_extract_syscall` strategy. From this point forward, memory allocations and page protections required during the reflective loading process are executed via dynamically resolved syscalls, entirely bypassing user-land hooks in `ntdll.dll`.

```c
snd_set_syscall_strategy(snd_hell_extract_syscall);
```

## 4. Execution

Like the other loaders, the context is populated with the Native APIs (`snd_mem_native`, `snd_mod_native`), and the target payload is loaded, mapped, and executed reflectively from memory.

## Building and Running

To compile this PoC specifically, ensure you are building in Release mode:

```bash
# Debug must be OFF to strip the CRT
cmake -B build -DSND_BUILD_PAYLOADS=ON -DSND_ENABLE_DEBUG=OFF
cmake --build build --config Release
```

Execution:
```bash
build\pocs\loader_noCRT_nowinapi\Release\loader_noCRT_nowinapi.exe
```
