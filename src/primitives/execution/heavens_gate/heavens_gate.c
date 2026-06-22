#include <sindri/common/status.h>
#include <sindri/primitives/heavens_gate.h>
#include <windows.h>

#if defined(_WIN64)

BOOL snd_is_wow64(void) {
    return FALSE;
}

snd_status_t snd_hg_execute_64(UINT64 pFunctionAddress, DWORD dwArgCount, const UINT64 *pArgs, UINT64 *pResult) {
    (void)pFunctionAddress;
    (void)dwArgCount;
    (void)pArgs;
    (void)pResult;
    return SND_ERR(SND_STATUS_UNSUPPORTED);
}

#elif defined(_WIN32)

/* -------------------------------------------------------------------------
 * External linkage: the MASM x86 Heaven's Gate bridge.
 * The ASM expects a strict 6-element array to prevent memory violations.
 * ------------------------------------------------------------------------- */
extern UINT64 snd_invoke_hg_x86(UINT64 pFunctionAddress, const UINT64 *pArgs);

BOOL snd_is_wow64(void) {
    void *pWow32Reserved = (void *)__readfsdword(0xC0);
    return (pWow32Reserved != NULL);
}

snd_status_t snd_hg_execute_64(UINT64 pFunctionAddress, DWORD dwArgCount, const UINT64 *pArgs, UINT64 *pResult) {
    if (pFunctionAddress == 0) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }

    // Enforce a maximum of 6 arguments to align with our static ASM wrapper
    if (dwArgCount > 6) {
        return SND_ERR_CTX(SND_STATUS_INVALID_PARAMETER, "Heaven's Gate bridge supports a maximum of 6 arguments.");
    }

    if (dwArgCount > 0 && pArgs == NULL) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }

    if (!snd_is_wow64()) {
        return SND_ERR(SND_STATUS_UNSUPPORTED);
    }

    UINT64 safe_args[6] = {0};
    for (DWORD i = 0; i < dwArgCount; i++) {
        safe_args[i] = pArgs[i];
    }

    UINT64 result = snd_invoke_hg_x86(pFunctionAddress, safe_args);
    if (pResult) {
        *pResult = result;
    }

    return SND_OK;
}

#else
#error "Unsupported architecture: SindriKit requires _WIN32 or _WIN64"
#endif
