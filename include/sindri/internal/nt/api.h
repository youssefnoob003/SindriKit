#ifndef SND_INTERNAL_NT_API_H
#define SND_INTERNAL_NT_API_H

#include <sindri/common/macros.h>
#include <sindri/internal/nt/types.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Type definition for LdrLoadDll function.
 */
typedef NTSTATUS(NTAPI *SND_LdrLoadDll_t)(PWSTR PathToFile, PULONG Flags, PSND_UNICODE_STRING ModuleFileName,
                                          PHANDLE ModuleHandle);

/*
 * @brief Type definition for NtOpenSection function.
 */
typedef NTSTATUS(NTAPI *SND_NtOpenSection_t)(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess,
                                             PSND_OBJECT_ATTRIBUTES ObjectAttributes);

/*
 * @brief Type definition for NtMapViewOfSection function.
 */
typedef NTSTATUS(NTAPI *SND_NtMapViewOfSection_t)(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress,
                                                  ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset,
                                                  PSIZE_T ViewSize, DWORD InheritDisposition, ULONG AllocationType,
                                                  ULONG Win32Protect);

/*
 * @brief Type definition for NtClose function.
 */
typedef NTSTATUS(NTAPI *SND_NtClose_t)(HANDLE Handle);

/*
 * @brief Type definition for NtAllocateVirtualMemory function.
 */
typedef NTSTATUS(NTAPI *SND_NtAllocateVirtualMemory_t)(HANDLE ProcessHandle, PVOID *BaseAddress, ULONG_PTR ZeroBits,
                                                       SIZE_T *Size, ULONG AllocationType, ULONG Win32Protect);

/*
 * @brief Type definition for NtProtectVirtualMemory function.
 */
typedef NTSTATUS(NTAPI *SND_NtProtectVirtualMemory_t)(HANDLE ProcessHandle, PVOID *BaseAddress, PSIZE_T RegionSize,
                                                      ULONG NewProtect, PULONG OldProtect);

/*
 * @brief Type definition for NtFreeVirtualMemory function.
 */
typedef NTSTATUS(NTAPI *SND_NtFreeVirtualMemory_t)(HANDLE ProcessHandle, PVOID *BaseAddress, PSIZE_T RegionSize,
                                                   ULONG FreeType);

/*
 * @brief Type definition for NtOpenProcess function.
 */
typedef NTSTATUS(NTAPI *SND_NtOpenProcess_t)(HANDLE *ProcessHandle, ACCESS_MASK DesiredAccess, PVOID ObjectAttributes,
                                             PVOID ClientId);

/*
 * @brief Type definition for NtWriteVirtualMemory function.
 */
typedef NTSTATUS(NTAPI *SND_NtWriteVirtualMemory_t)(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T Size,
                                                    SIZE_T *NumberOfBytesWritten);

/*
 * @brief Type definition for NtCreateThreadEx function.
 */
typedef NTSTATUS(NTAPI *SND_NtCreateThreadEx_t)(HANDLE *ThreadHandle, ACCESS_MASK DesiredAccess, PVOID ObjectAttributes,
                                                HANDLE ProcessHandle, PVOID StartRoutine, PVOID Argument,
                                                ULONG CreateFlags, SIZE_T ZeroBits, SIZE_T StackSize,
                                                SIZE_T MaximumStackSize, PVOID AttributeList);

SND_END_EXTERN_C

#endif // SND_INTERNAL_NT_API_H
