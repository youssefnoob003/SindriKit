#ifndef SND_INTERNAL_WIN32_CONSTANTS_H
#define SND_INTERNAL_WIN32_CONSTANTS_H

/* Win32-only CreateFile and Heap flags. PAGE_*, MEM_*, and GENERIC_* live in
 * internal/windows/constants.h because NT and syscall backends use them too. */

#include <sindri/internal/windows/constants.h>

#define SND_OPEN_EXISTING         0x00000003UL
#define SND_FILE_ATTRIBUTE_NORMAL 0x00000080UL
#define SND_HEAP_ZERO_MEMORY      0x00000008UL

#endif /* SND_INTERNAL_WIN32_CONSTANTS_H */
