#ifndef SND_PRIMITIVES_FFI_H
#define SND_PRIMITIVES_FFI_H

#include <sindri/common/macros.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Invokes an arbitrary function pointer at runtime using a generic
 * argument array.
 *
 * @param pFunctionAddress  Pointer to the target function. Must not be NULL;
 * passing NULL causes an immediate return of 0.
 * @param dwArgCount        Number of entries in @p pArgs. May be 0 for
 * zero-argument calls.
 * @param pArgs             Array of @p dwArgCount UINT_PTR-sized arguments.
 * Ignored (and may be NULL) when @p dwArgCount is 0.
 *
 * @return The value returned by the invoked function as a UINT_PTR, or 0 if
 * @p pFunctionAddress is NULL or the current architecture is unsupported.
 *
 * @note   Callee-saved registers (RBX, RBP, RDI, RSI, R12-R15) are preserved
 * by the assembly bridge. The caller is responsible for ensuring that
 * the argument types and count are compatible with the target function.
 */
UINT_PTR snd_ffi_execute(PVOID pFunctionAddress, DWORD dwArgCount, const UINT_PTR *pArgs);

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_FFI_H
