#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/internal/nt/api.h>
#include <sindri/parsers/env/peb.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/primitives/os_api.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri_hashes.h>
#include <windows.h>
#include <winerror.h>

static snd_status_t WINAPI nt_queue_apc(HANDLE thread, PVOID apc_routine, PVOID apc_argument) {
    if (!thread || !apc_routine)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status = snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTQUEUEAPCTHREAD, &func_addr, NULL);
    if (SND_FAILED(status))
        return status;

    SND_NtQueueApcThread_t pNtQueueApcThread = (SND_NtQueueApcThread_t)func_addr;
    NTSTATUS nt_status = pNtQueueApcThread(thread, apc_routine, apc_argument, NULL, NULL);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_APC_QUEUE_FAILED, nt_status);
}

static snd_status_t WINAPI nt_resume_thread(HANDLE thread) {
    if (!thread)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status = snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTRESUMETHREAD, &func_addr, NULL);
    if (SND_FAILED(status))
        return status;

    ULONG previous_suspend_count = 0;
    SND_NtResumeThread_t pNtResumeThread = (SND_NtResumeThread_t)func_addr;
    NTSTATUS nt_status = pNtResumeThread(thread, &previous_suspend_count);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_RESUME_FAILED, nt_status);
}

static snd_status_t WINAPI nt_suspend_thread(HANDLE thread) {
    if (!thread)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status = snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTSUSPENDTHREAD, &func_addr, NULL);
    if (SND_FAILED(status))
        return status;

    ULONG previous_suspend_count = 0;
    SND_NtSuspendThread_t pNtSuspendThread = (SND_NtSuspendThread_t)func_addr;
    NTSTATUS nt_status = pNtSuspendThread(thread, &previous_suspend_count);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_SUSPEND_FAILED, nt_status);
}

static snd_status_t WINAPI nt_close_thread(HANDLE handle) {
    if (!handle)
        return SND_OK;

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status = snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTCLOSE, &func_addr, NULL);
    if (SND_FAILED(status))
        return status;

    SND_NtClose_t pNtClose  = (SND_NtClose_t)func_addr;
    NTSTATUS      nt_status = pNtClose(handle);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_HANDLE_CLOSE_FAILED, nt_status);
}

const snd_thread_api_t snd_thread_nt = {
    .queue_apc      = nt_queue_apc,
    .resume_thread  = nt_resume_thread,
    .suspend_thread = nt_suspend_thread,
    .close_handle   = nt_close_thread
};
