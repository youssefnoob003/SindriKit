#ifndef SND_INTERNAL_WINDOWS_CONSTANTS_H
#define SND_INTERNAL_WINDOWS_CONSTANTS_H

/* Shared Windows ABI constants used by Win32, NT, and syscall backends.
 * Win32-only CreateFile/Heap flags stay in internal/win32/constants.h. */

#define SND_MEM_RELEASE 0x8000
#define SND_MEM_COMMIT  0x1000
#define SND_MEM_RESERVE 0x2000

#define SND_PAGE_NOACCESS          0x01
#define SND_PAGE_READONLY          0x02
#define SND_PAGE_READWRITE         0x04
#define SND_PAGE_EXECUTE           0x10
#define SND_PAGE_EXECUTE_READ      0x20
#define SND_PAGE_EXECUTE_READWRITE 0x40

#define SND_GENERIC_READ    0x80000000UL
#define SND_FILE_SHARE_READ 0x00000001UL

#define SND_DLL_PROCESS_ATTACH 1
#define SND_DLL_PROCESS_DETACH 0

#endif /* SND_INTERNAL_WINDOWS_CONSTANTS_H */
