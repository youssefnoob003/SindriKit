#include <sindri/internal/nt_defs.h>
#include <sindri/loaders/knowndlls/knowndlls.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>
#include <windows.h>

#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#endif

static snd_status_t WINAPI native_NtOpenSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess,
                                                PSND_OBJECT_ATTRIBUTES ObjectAttributes) {
    snd_syscall_entry_t entry          = {0};
    snd_status_t        resolve_status = snd_resolve_syscall(SND_HASH_NTOPENSECTION, &entry);
    if (resolve_status.code != SND_SUCCESS)
        return resolve_status;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.arg1               = SectionHandle;
    args.arg2               = (PVOID)(ULONG_PTR)DesiredAccess;
    args.arg3               = ObjectAttributes;

    NTSTATUS status = snd_invoke_syscall_asm(&args);
    return NT_SUCCESS(status) ? SND_OK : SND_ERR_CTX(SND_ERROR_GENERIC, "NtOpenSection failed via native syscall");
}

static snd_status_t WINAPI native_NtMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress,
                                                     ULONG_PTR ZeroBits, SIZE_T CommitSize,
                                                     PLARGE_INTEGER SectionOffset, PSIZE_T ViewSize,
                                                     DWORD InheritDisposition, ULONG AllocationType,
                                                     ULONG Win32Protect) {
    snd_syscall_entry_t entry          = {0};
    snd_status_t        resolve_status = snd_resolve_syscall(SND_HASH_NTMAPVIEWOFSECTION, &entry);
    if (resolve_status.code != SND_SUCCESS)
        return resolve_status;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.arg1               = SectionHandle;
    args.arg2               = ProcessHandle;
    args.arg3               = BaseAddress;
    args.arg4               = (PVOID)ZeroBits;
    args.arg5               = (PVOID)CommitSize;
    args.arg6               = SectionOffset;
    args.arg7               = ViewSize;
    args.arg8               = (PVOID)(ULONG_PTR)InheritDisposition;
    args.arg9               = (PVOID)(ULONG_PTR)AllocationType;
    args.arg10              = (PVOID)(ULONG_PTR)Win32Protect;

    NTSTATUS status = snd_invoke_syscall_asm(&args);
    return NT_SUCCESS(status) ? SND_OK : SND_ERR_CTX(SND_ERROR_GENERIC, "NtMapViewOfSection failed via native syscall");
}

static snd_status_t WINAPI native_NtClose(HANDLE Handle) {
    snd_syscall_entry_t entry          = {0};
    snd_status_t        resolve_status = snd_resolve_syscall(SND_HASH_NTCLOSE, &entry);
    if (resolve_status.code != SND_SUCCESS)
        return resolve_status;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.arg1               = Handle;

    NTSTATUS status = snd_invoke_syscall_asm(&args);
    return NT_SUCCESS(status) ? SND_OK : SND_ERR_CTX(SND_ERROR_GENERIC, "NtClose failed via native syscall");
}

const snd_knowndlls_config_t snd_knowndlls_native = {.open_section        = native_NtOpenSection,
                                                     .map_view_of_section = native_NtMapViewOfSection,
                                                     .close_handle        = native_NtClose};
