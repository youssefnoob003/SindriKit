#ifndef SND_INTERNAL_NEIGHBOR_H
#define SND_INTERNAL_NEIGHBOR_H

#include <sindri/common/status.h>
#include <sindri/internal/nt_defs.h>
#include <sindri/primitives/syscalls.h>
#include <windows.h>

/**
 * @brief Shared neighbor-search loop for hooked syscall stubs.
 * @param p Pointer to the current byte being examined.
 * @param ntdll Base address of the ntdll module.
 * @param image_size Size of the ntdll module image.
 * @param ssn_out Pointer to receive the resolved SSN.
 * @return SND_OK on success, or an error status.
 */
snd_status_t snd_neighbor_search_ssn(BYTE *p, PVOID ntdll, SIZE_T image_size, WORD *ssn_out);

#endif // SND_INTERNAL_NEIGHBOR_H
