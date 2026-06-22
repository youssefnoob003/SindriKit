#include <sindri/common/hash.h>
#include <sindri/common/helpers.h>
#include <sindri/internal/nt_defs.h>
#include <sindri/primitives/memory.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>
#include <windows.h>

static snd_status_t WINAPI native_alloc(LPVOID address, SIZE_T size, DWORD allocation_type, DWORD protect,
                                        LPVOID *out_address) {
    if (!out_address)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    *out_address                       = NULL;
    snd_syscall_entry_t entry          = {0};
    snd_status_t        resolve_status = snd_resolve_syscall(SND_HASH_NTALLOCATEVIRTUALMEMORY, &entry);
    if (resolve_status.code != SND_SUCCESS)
        return resolve_status;

    HANDLE processHandle = (HANDLE)-1;
    PVOID  baseAddress   = address;
    SIZE_T regionSize    = size;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.arg1               = processHandle;
    args.arg2               = &baseAddress;
    args.arg3               = 0;
    args.arg4               = &regionSize;
    args.arg5               = (PVOID)(ULONG_PTR)allocation_type;
    args.arg6               = (PVOID)(ULONG_PTR)protect;

    NTSTATUS status = snd_invoke_syscall_asm(&args);
    if (NT_SUCCESS(status)) {
        *out_address = baseAddress;
        return SND_OK;
    }
    return SND_ERR_CTX(SND_STATUS_ALLOC_FAILED, "NtAllocateVirtualMemory failed");
}

static snd_status_t WINAPI native_free(LPVOID address, SIZE_T size, DWORD free_type) {
    snd_syscall_entry_t entry          = {0};
    snd_status_t        resolve_status = snd_resolve_syscall(SND_HASH_NTFREEVIRTUALMEMORY, &entry);
    if (resolve_status.code != SND_SUCCESS)
        return resolve_status;

    HANDLE processHandle = (HANDLE)-1;
    PVOID  baseAddress   = address;
    SIZE_T regionSize    = size;

    if (free_type == MEM_RELEASE) {
        regionSize = 0;
    }

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.arg1               = processHandle;
    args.arg2               = &baseAddress;
    args.arg3               = &regionSize;
    args.arg4               = (PVOID)(ULONG_PTR)free_type;

    NTSTATUS status = snd_invoke_syscall_asm(&args);
    return NT_SUCCESS(status) ? SND_OK : SND_ERR_CTX(SND_ERROR_GENERIC, "NtFreeVirtualMemory failed");
}

static snd_status_t WINAPI native_protect(LPVOID address, SIZE_T size, DWORD new_protect, DWORD *old_protect) {
    snd_syscall_entry_t entry          = {0};
    snd_status_t        resolve_status = snd_resolve_syscall(SND_HASH_NTPROTECTVIRTUALMEMORY, &entry);
    if (resolve_status.code != SND_SUCCESS)
        return resolve_status;

    HANDLE processHandle = (HANDLE)-1;
    PVOID  baseAddress   = address;
    SIZE_T regionSize    = size;
    ULONG  oldProtect    = 0;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.arg1               = processHandle;
    args.arg2               = &baseAddress;
    args.arg3               = &regionSize;
    args.arg4               = (PVOID)(ULONG_PTR)new_protect;
    args.arg5               = &oldProtect;

    NTSTATUS status = snd_invoke_syscall_asm(&args);

    if (NT_SUCCESS(status)) {
        if (old_protect) {
            *old_protect = oldProtect;
        }
        return SND_OK;
    }
    return SND_ERR_CTX(SND_STATUS_PROTECTION_UPDATE_FAILED, "NtProtectVirtualMemory failed");
}

const snd_memory_api_t snd_mem_native = {.alloc = native_alloc, .free = native_free, .protect = native_protect};
