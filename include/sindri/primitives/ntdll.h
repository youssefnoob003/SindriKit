#ifndef SND_PRIMITIVES_NTDLL_H
#define SND_PRIMITIVES_NTDLL_H

#include <sindri/common/helpers.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Sets the global base address of the ntdll.dll module.
 * @param ntdll_base The base address of the ntdll module.
 */
void snd_set_ntdll(PVOID ntdll_base);

/**
 * @brief Retrieves the global base address of the ntdll.dll module.
 * @return The base address of the ntdll module, or NULL if not set.
 */
PVOID snd_get_ntdll(void);

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_NTDLL_H
