#ifndef SND_INTERNAL_WIN32_API_H
#define SND_INTERNAL_WIN32_API_H

#include <sindri/internal/windows/types.h>

#if !defined(SND_USE_WINDOWS_SDK)
HANDLE WINAPI GetProcessHeap(void);
HANDLE WINAPI CreateFileA(const char *name, DWORD desired_access, DWORD share_mode, PVOID security_attributes,
                          DWORD creation_disposition, DWORD flags_and_attributes, HANDLE template_file);
PVOID WINAPI  HeapAlloc(HANDLE heap, DWORD flags, SIZE_T size);
BOOL WINAPI   HeapFree(HANDLE heap, DWORD flags, PVOID memory);
BOOL WINAPI   ReadFile(HANDLE file, PVOID buffer, DWORD bytes_to_read, PDWORD bytes_read, PVOID overlapped);
BOOL WINAPI   CloseHandle(HANDLE handle);
DWORD WINAPI  GetLastError(void);
BOOL WINAPI   VirtualFree(PVOID address, SIZE_T size, DWORD free_type);
BOOL WINAPI   UnmapViewOfFile(const void *base_address);
BOOL WINAPI   GetFileSizeEx(HANDLE file, PSND_LARGE_INTEGER size);
VOID WINAPI   OutputDebugStringA(const char *output_string);
#endif /* !SND_USE_WINDOWS_SDK */

#endif /* SND_INTERNAL_WIN32_API_H */
