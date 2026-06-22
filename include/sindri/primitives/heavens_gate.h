#ifndef SND_PRIMITIVES_HEAVENS_GATE_H
#define SND_PRIMITIVES_HEAVENS_GATE_H

#include <sindri/common/helpers.h>
#include <sindri/common/status.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Checks if the current 32-bit process is running under WOW64 (64-bit
 * OS).
 * @note This reads the WOW32Reserved field directly from the TEB.
 * @return TRUE if running under WOW64, FALSE otherwise.
 */
BOOL snd_is_wow64(void);

/**
 * @brief Invokes a 64-bit function from a 32-bit WOW64 process using Heaven's
 * Gate.
 *
 * @param pFunctionAddress The 64-bit virtual address of the target function.
 * @param dwArgCount Number of arguments to pass to the 64-bit function (Max:
 * 6).
 * @param pArgs Array of 64-bit arguments.
 * @param pResult Pointer to store the 64-bit return value (RAX).
 * @return SND_OK on success, or an appropriate error code if unsupported.
 */
snd_status_t snd_hg_execute_64(UINT64 pFunctionAddress, DWORD dwArgCount, const UINT64 *pArgs, UINT64 *pResult);

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_HEAVENS_GATE_H
