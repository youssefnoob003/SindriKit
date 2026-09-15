#include <sindri/internal/nt/api.h>
#include <sindri/internal/windows/types.h>
#include <sindri/parsers/env/ntdll.h>
#include <sindri/primitives/os_api.h>
#include <sindri/primitives/status.h>
#include <sindri_hashes.h>

static snd_status_t WINAPI nt_queue_apc(HANDLE thread, PVOID apc_routine, PVOID apc_argument) {
    SND_CHECK_NULL(thread, apc_routine);

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTQUEUEAPCTHREAD, &func_addr));

    SND_NtQueueApcThread_t pNtQueueApcThread = (SND_NtQueueApcThread_t)func_addr;
    NTSTATUS               nt_status         = pNtQueueApcThread(thread, apc_routine, apc_argument, NULL, NULL);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_QUEUE_FAILED, nt_status);
}

static snd_status_t WINAPI nt_resume_thread(HANDLE thread) {
    SND_CHECK_NULL(thread);

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTRESUMETHREAD, &func_addr));

    ULONG                previous_suspend_count = 0;
    SND_NtResumeThread_t pNtResumeThread        = (SND_NtResumeThread_t)func_addr;
    NTSTATUS             nt_status              = pNtResumeThread(thread, &previous_suspend_count);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_RESUME_FAILED, nt_status);
}

static snd_status_t WINAPI nt_suspend_thread(HANDLE thread) {
    SND_CHECK_NULL(thread);

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTSUSPENDTHREAD, &func_addr));

    ULONG                 previous_suspend_count = 0;
    SND_NtSuspendThread_t pNtSuspendThread       = (SND_NtSuspendThread_t)func_addr;
    NTSTATUS              nt_status              = pNtSuspendThread(thread, &previous_suspend_count);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_SUSPEND_FAILED, nt_status);
}

static snd_status_t WINAPI nt_close_thread(HANDLE handle) {
    if (!handle) {
        return SND_OK;
    }

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTCLOSE, &func_addr));

    SND_NtClose_t pNtClose  = (SND_NtClose_t)func_addr;
    NTSTATUS      nt_status = pNtClose(handle);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_HANDLE_CLOSE_FAILED, nt_status);
}

const snd_thread_api_t snd_thread_nt = {.queue_apc      = nt_queue_apc,
                                        .resume_thread  = nt_resume_thread,
                                        .suspend_thread = nt_suspend_thread,
                                        .close_handle   = nt_close_thread};
