#ifndef SND_INTERNAL_NT_API_H
#define SND_INTERNAL_NT_API_H

#include <sindri/common/macros.h>
#include <sindri/internal/nt/base.h>
#include <sindri/internal/nt/file.h>
#include <sindri/internal/nt/process.h>

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
                                                  ULONG_PTR ZeroBits, SIZE_T CommitSize,
                                                  PSND_LARGE_INTEGER SectionOffset, PSIZE_T ViewSize,
                                                  DWORD InheritDisposition, ULONG AllocationType, ULONG Win32Protect);

/*
 * @brief Type definition for NtClose function.
 */
typedef NTSTATUS(NTAPI *SND_NtClose_t)(HANDLE Handle);

typedef NTSTATUS(NTAPI *SND_NtCreateFile_t)(PHANDLE FileHandle, ACCESS_MASK DesiredAccess,
                                            PSND_OBJECT_ATTRIBUTES ObjectAttributes, SND_IO_STATUS_BLOCK *IoStatusBlock,
                                            PSND_LARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess,
                                            ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer,
                                            ULONG EaLength);

typedef NTSTATUS(NTAPI *SND_NtReadFile_t)(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext,
                                          SND_IO_STATUS_BLOCK *IoStatusBlock, PVOID Buffer, ULONG Length,
                                          PSND_LARGE_INTEGER ByteOffset, PULONG Key);

typedef NTSTATUS(NTAPI *SND_NtQueryInformationFile_t)(HANDLE FileHandle, SND_IO_STATUS_BLOCK *IoStatusBlock,
                                                      PVOID FileInformation, ULONG Length, ULONG FileInformationClass);

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
 * @brief Type definition for NtOpenThread function.
 */
typedef NTSTATUS(NTAPI *SND_NtOpenThread_t)(PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess, PVOID ObjectAttributes,
                                            PVOID ClientId);

/*
 * @brief Type definition for NtGetContextThread function.
 */
typedef NTSTATUS(NTAPI *SND_NtGetContextThread_t)(HANDLE ThreadHandle, PVOID ThreadContext);

/*
 * @brief Type definition for NtSetContextThread function.
 */
typedef NTSTATUS(NTAPI *SND_NtSetContextThread_t)(HANDLE ThreadHandle, PVOID ThreadContext);

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

/*
 * @brief Type definition for NtQueueApcThread function.
 */
typedef NTSTATUS(NTAPI *SND_NtQueueApcThread_t)(HANDLE ThreadHandle, PVOID ApcRoutine, PVOID ApcRoutineContext,
                                                PVOID ApcStatusBlock, PVOID ApcReserved);

/*
 * @brief Type definition for NtResumeThread function.
 */
typedef NTSTATUS(NTAPI *SND_NtResumeThread_t)(HANDLE ThreadHandle, PULONG PreviousSuspendCount);

/*
 * @brief Type definition for NtSuspendThread function.
 */
typedef NTSTATUS(NTAPI *SND_NtSuspendThread_t)(HANDLE ThreadHandle, PULONG PreviousSuspendCount);

/*
 * @brief Type definition for RtlCreateProcessParametersEx function.
 */
typedef NTSTATUS(NTAPI *SND_RtlCreateProcessParametersEx_t)(
    PVOID *ProcessParameters, PSND_UNICODE_STRING ImagePathName, PSND_UNICODE_STRING DllPath,
    PSND_UNICODE_STRING CurrentDirectory, PSND_UNICODE_STRING CommandLine, PVOID Environment,
    PSND_UNICODE_STRING WindowTitle, PSND_UNICODE_STRING DesktopInfo, PSND_UNICODE_STRING ShellInfo,
    PSND_UNICODE_STRING RuntimeData, ULONG Flags);

/*
 * @brief Type definition for RtlDestroyProcessParameters function.
 */
typedef NTSTATUS(NTAPI *SND_RtlDestroyProcessParameters_t)(PVOID ProcessParameters);

typedef VOID(NTAPI *SND_RtlExitUserProcess_t)(NTSTATUS ExitStatus);

/*
 * @brief Type definition for NtCreateUserProcess function.
 */
typedef NTSTATUS(NTAPI *SND_NtCreateUserProcess_t)(
    PHANDLE ProcessHandle, PHANDLE ThreadHandle, ACCESS_MASK ProcessDesiredAccess, ACCESS_MASK ThreadDesiredAccess,
    PSND_OBJECT_ATTRIBUTES ProcessObjectAttributes, PSND_OBJECT_ATTRIBUTES ThreadObjectAttributes, ULONG ProcessFlags,
    ULONG ThreadFlags, PVOID ProcessParameters, PSND_PS_CREATE_INFO CreateInfo, PSND_PS_ATTRIBUTE_LIST AttributeList);

SND_END_EXTERN_C

#endif // SND_INTERNAL_NT_API_H
