#include <sindri/common/hash.h>
#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/internal/nt/peb.h>
#include <sindri/primitives/memory.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>
#include <windows.h>

extern snd_syscall_invoker_t g_syscall_invoker;

static snd_status_t WINAPI sys_alloc(LPVOID address, SIZE_T size, DWORD allocation_type, DWORD protect,
                                     LPVOID *out_address) {
    if (!out_address)
        return SND_ERR(SND_STATUS_NULL_POINTER);
    *out_address               = NULL;
    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTALLOCATEVIRTUALMEMORY, &entry);
    if (SND_FAILED(status))
        return status;

    HANDLE processHandle = (HANDLE)-1;
    PVOID  baseAddress   = address;
    SIZE_T regionSize    = size;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.spoof_addr         = entry.pSpoofAddr;
    args.spoof_frame_size   = entry.dwSpoofFrameSize;
    args.arg1               = processHandle;
    args.arg2               = &baseAddress;
    args.arg3               = 0;
    args.arg4               = &regionSize;
    args.arg5               = (PVOID)(ULONG_PTR)allocation_type;
    args.arg6               = (PVOID)(ULONG_PTR)protect;

    if (g_syscall_invoker == NULL) {
        return SND_ERR_CTX(SND_STATUS_NOT_INITIALIZED, "g_syscall_invoker is NULL");
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    if (SND_NT_SUCCESS(nt_status)) {
        *out_address = baseAddress;
        return SND_OK;
    }
    return SND_ERR_NT(SND_STATUS_ALLOC_FAILED, nt_status);
}

static snd_status_t WINAPI sys_free(LPVOID address, SIZE_T size, DWORD free_type) {
    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTFREEVIRTUALMEMORY, &entry);
    if (SND_FAILED(status))
        return status;

    HANDLE processHandle = (HANDLE)-1;
    PVOID  baseAddress   = address;
    SIZE_T regionSize    = size;

    if (free_type == MEM_RELEASE) {
        regionSize = 0;
    }

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.spoof_addr         = entry.pSpoofAddr;
    args.spoof_frame_size   = entry.dwSpoofFrameSize;
    args.arg1               = processHandle;
    args.arg2               = &baseAddress;
    args.arg3               = &regionSize;
    args.arg4               = (PVOID)(ULONG_PTR)free_type;

    if (g_syscall_invoker == NULL) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_VIRTUAL_FREE_FAILED, nt_status);
}

static snd_status_t WINAPI sys_protect(LPVOID address, SIZE_T size, DWORD new_protect, DWORD *old_protect) {
    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTPROTECTVIRTUALMEMORY, &entry);
    if (SND_FAILED(status))
        return status;

    HANDLE processHandle = (HANDLE)-1;
    PVOID  baseAddress   = address;
    SIZE_T regionSize    = size;
    ULONG  oldProtect    = 0;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.spoof_addr         = entry.pSpoofAddr;
    args.spoof_frame_size   = entry.dwSpoofFrameSize;
    args.arg1               = processHandle;
    args.arg2               = &baseAddress;
    args.arg3               = &regionSize;
    args.arg4               = (PVOID)(ULONG_PTR)new_protect;
    args.arg5               = &oldProtect;

    if (g_syscall_invoker == NULL) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);

    if (SND_NT_SUCCESS(nt_status)) {
        if (old_protect) {
            *old_protect = oldProtect;
        }
        return SND_OK;
    }
    return SND_ERR_NT(SND_STATUS_PROTECTION_UPDATE_FAILED, nt_status);
}

const snd_memory_api_t snd_mem_sys = {.alloc = sys_alloc, .free = sys_free, .protect = sys_protect};
