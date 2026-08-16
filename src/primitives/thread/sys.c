#include <sindri/common/status.h>
#include <sindri/internal/nt/types.h>
#include <sindri/primitives/os_api.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>
#include <windows.h>

extern snd_syscall_invoker_t g_syscall_invoker;

static snd_status_t WINAPI sys_queue_apc(HANDLE thread, PVOID apc_routine, PVOID apc_argument) {
    if (!thread || !apc_routine)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTQUEUEAPCTHREAD, &entry);
    if (SND_FAILED(status))
        return status;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.spoof_addr         = entry.pSpoofAddr;
    args.spoof_frame_size   = entry.dwSpoofFrameSize;
    args.arg1               = thread;
    args.arg2               = apc_routine;
    args.arg3               = apc_argument;
    args.arg4               = NULL;
    args.arg5               = NULL;

    if (g_syscall_invoker == NULL) {
        return SND_ERR_CTX(SND_STATUS_NOT_INITIALIZED, "g_syscall_invoker is NULL");
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_APC_QUEUE_FAILED, nt_status);
}

static snd_status_t WINAPI sys_resume_thread(HANDLE thread) {
    if (!thread)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTRESUMETHREAD, &entry);
    if (SND_FAILED(status))
        return status;

    ULONG previous_suspend_count = 0;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.spoof_addr         = entry.pSpoofAddr;
    args.spoof_frame_size   = entry.dwSpoofFrameSize;
    args.arg1               = thread;
    args.arg2               = &previous_suspend_count;

    if (g_syscall_invoker == NULL) {
        return SND_ERR_CTX(SND_STATUS_NOT_INITIALIZED, "g_syscall_invoker is NULL");
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_RESUME_FAILED, nt_status);
}

static snd_status_t WINAPI sys_suspend_thread(HANDLE thread) {
    if (!thread)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTSUSPENDTHREAD, &entry);
    if (SND_FAILED(status))
        return status;

    ULONG previous_suspend_count = 0;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.spoof_addr         = entry.pSpoofAddr;
    args.spoof_frame_size   = entry.dwSpoofFrameSize;
    args.arg1               = thread;
    args.arg2               = &previous_suspend_count;

    if (g_syscall_invoker == NULL) {
        return SND_ERR_CTX(SND_STATUS_NOT_INITIALIZED, "g_syscall_invoker is NULL");
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_SUSPEND_FAILED, nt_status);
}

static snd_status_t WINAPI sys_close_thread(HANDLE handle) {
    if (!handle)
        return SND_OK;

    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTCLOSE, &entry);
    if (SND_FAILED(status))
        return status;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.spoof_addr         = entry.pSpoofAddr;
    args.spoof_frame_size   = entry.dwSpoofFrameSize;
    args.arg1               = handle;

    if (g_syscall_invoker == NULL) {
        return SND_ERR_CTX(SND_STATUS_NOT_INITIALIZED, "g_syscall_invoker is NULL");
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_HANDLE_CLOSE_FAILED, nt_status);
}

const snd_thread_api_t snd_thread_sys = {
    .queue_apc      = sys_queue_apc,
    .resume_thread  = sys_resume_thread,
    .suspend_thread = sys_suspend_thread,
    .close_handle   = sys_close_thread
};
