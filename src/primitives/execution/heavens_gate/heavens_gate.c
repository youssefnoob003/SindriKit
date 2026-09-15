#include <sindri/internal/windows/types.h>
#include <sindri/parsers/env/peb.h>
#include <sindri/primitives/heavens_gate.h>
#include <sindri/status.h>
#include <sindri/status/core.h>

#if defined(_WIN64)

BOOL snd_is_wow64(void) {
    return FALSE;
}

snd_status_t snd_hg_execute_64(ULONGLONG pFunctionAddress, DWORD dwArgCount, const ULONGLONG *pArgs,
                               ULONGLONG *pResult) {
    (void)pFunctionAddress;
    (void)dwArgCount;
    (void)pArgs;
    (void)pResult;
    return SND_ERR(SND_STATUS_ARCH_MISMATCH);
}

#elif defined(_WIN32)

/* -------------------------------------------------------------------------
 * External linkage: the MASM x86 Heaven's Gate bridge.
 * The ASM expects a strict 6-element array to prevent memory violations.
 * ------------------------------------------------------------------------- */
extern ULONGLONG snd_hg_invoke_x86(ULONGLONG pFunctionAddress, const ULONGLONG *pArgs);

BOOL snd_is_wow64(void) {
    return (snd_env_get_wow32_reserved() != NULL);
}

snd_status_t snd_hg_execute_64(ULONGLONG pFunctionAddress, DWORD dwArgCount, const ULONGLONG *pArgs,
                               ULONGLONG *pResult) {
    SND_CHECK_NULL(pFunctionAddress);

    // Enforce maximum argument count supported by static ASM wrapper
    if (dwArgCount > SND_HG_MAX_ARGS) {
        return SND_ERR_CTX(SND_STATUS_TOO_MANY_ARGUMENTS, "Heaven's Gate bridge supports a maximum of %d arguments.",
                           SND_HG_MAX_ARGS);
    }

    if (dwArgCount > 0 && pArgs == NULL) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETERS_COMBINATION);
    }

    if (!snd_is_wow64()) {
        return SND_ERR(SND_STATUS_ARCH_MISMATCH);
    }

    ULONGLONG safe_args[SND_HG_MAX_ARGS] = {0};
    for (DWORD i = 0; i < dwArgCount; i++) {
        safe_args[i] = pArgs[i];
    }

    ULONGLONG result = snd_hg_invoke_x86(pFunctionAddress, safe_args);
    if (pResult) {
        *pResult = result;
    }

    return SND_OK;
}

#else
#error "Unsupported architecture: SindriKit requires _WIN32 or _WIN64"
#endif
