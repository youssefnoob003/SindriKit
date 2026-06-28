#include <sindri/primitives/ffi.h>
#include <windows.h>

/* -------------------------------------------------------------------------
 * External linkage: the MASM x64 / x86 bridges.
 * ------------------------------------------------------------------------- */
#if defined(_WIN64)
extern UINT_PTR snd_ffi_bridge_x64(PVOID pFunc, const UINT_PTR *pArgs, DWORD dwCount);
#elif defined(_WIN32)
extern UINT_PTR snd_ffi_bridge_x86(PVOID pFunc, const UINT_PTR *pArgs, DWORD dwCount);
#else
#error "Unsupported architecture: SindriKit requires _WIN32 or _WIN64"
#endif

/* -------------------------------------------------------------------------
 * snd_ffi_execute
 *
 * Validates inputs, then routes to the architecture-specific trampoline.
 * Uses PVOID for the function address to avoid MSVC C4152
 * (non-standard function pointer conversion) on the call site. The actual
 * type-pun into a callable pointer is done inside the assembly stub where
 * the CPU simply JMPs/CALLs the pointer value, bypassing the C type system.
 * ------------------------------------------------------------------------- */
UINT_PTR snd_ffi_execute(PVOID pFunctionAddress, DWORD dwArgCount, const UINT_PTR *pArgs) {
    /* Null function pointer: graceful no-op. */
    if (pFunctionAddress == NULL) {
        return 0;
    }

    /* If arguments are expected but the array pointer is NULL, refuse. */
    if (dwArgCount > 0 && pArgs == NULL) {
        return 0;
    }

#if defined(_WIN64)
    return snd_ffi_bridge_x64(pFunctionAddress, pArgs, dwArgCount);
#elif defined(_WIN32)
    return snd_ffi_bridge_x86(pFunctionAddress, pArgs, dwArgCount);
#else
#error "Unsupported architecture: SindriKit requires _WIN32 or _WIN64"
#endif
}
