#include <sindri/common/status.h>
#include <sindri/primitives/os_api.h>
#include <windows.h>

static snd_status_t WINAPI win_queue_apc(HANDLE thread, PVOID apc_routine, PVOID apc_argument) {
    if (!thread || !apc_routine)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    DWORD result = QueueUserAPC((PAPCFUNC)apc_routine, thread, (ULONG_PTR)apc_argument);
    return result != 0 ? SND_OK : SND_ERR_W32(SND_STATUS_APC_QUEUE_FAILED);
}

static snd_status_t WINAPI win_resume_thread(HANDLE thread) {
    if (!thread)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    DWORD suspend_count = ResumeThread(thread);
    return suspend_count != (DWORD)-1 ? SND_OK : SND_ERR_W32(SND_STATUS_THREAD_RESUME_FAILED);
}

static snd_status_t WINAPI win_suspend_thread(HANDLE thread) {
    if (!thread)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    DWORD suspend_count = SuspendThread(thread);
    return suspend_count != (DWORD)-1 ? SND_OK : SND_ERR_W32(SND_STATUS_THREAD_SUSPEND_FAILED);
}

static snd_status_t WINAPI win_close_thread(HANDLE handle) {
    return CloseHandle(handle) ? SND_OK : SND_ERR(SND_STATUS_HANDLE_CLOSE_FAILED);
}

const snd_thread_api_t snd_thread_win = {
    .queue_apc      = win_queue_apc,
    .resume_thread  = win_resume_thread,
    .suspend_thread = win_suspend_thread,
    .close_handle   = win_close_thread
};
