#ifndef SND_PRIMITIVES_HEAVENS_GATE_H
#define SND_PRIMITIVES_HEAVENS_GATE_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/types.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Maximum number of 64-bit arguments supported by the Heaven's Gate invocation bridge.
 * @note Matches the static stack reservation in the MASM x86 assembly stub (heavens_gate_x86.asm).
 */
#define SND_HG_MAX_ARGS 6

/**
 * @brief Checks if the current 32-bit process is running under WOW64 (64-bit OS).
 * @note Reads the WOW32Reserved field directly from the 32-bit TEB (fs:[0xC0]).
 * @retval TRUE If running under WOW64.
 * @retval FALSE Otherwise.
 */
BOOL snd_is_wow64(void);

/**
 * @brief Invokes a 64-bit function from a 32-bit WOW64 process via Heaven's Gate.
 * Transitions CS selector from 0x23 to 0x33 via FAR RET.
 *
 * @param pFunctionAddress The 64-bit virtual address of the target function.
 * @param dwArgCount Number of arguments to pass to the function (Max: SND_HG_MAX_ARGS).
 * @param pArgs Array of 64-bit arguments.
 * @param pResult Pointer to store the 64-bit return value (RAX).
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p pFunctionAddress is zero.
 * @retval SND_STATUS_TOO_MANY_ARGUMENTS If @p dwArgCount exceeds
 * `SND_HG_MAX_ARGS`.
 * @retval SND_STATUS_INVALID_PARAMETERS_COMBINATION If @p dwArgCount is
 * nonzero and @p pArgs is NULL.
 * @retval SND_STATUS_ARCH_MISMATCH If the process is not running under WOW64.
 */
snd_status_t snd_hg_execute_64(ULONGLONG pFunctionAddress, DWORD dwArgCount, const ULONGLONG *pArgs,
                               ULONGLONG *pResult);

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_HEAVENS_GATE_H
