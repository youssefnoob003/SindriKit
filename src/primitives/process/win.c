#include <sindri/common/status.h>
#include <sindri/primitives/process.h>
#include <windows.h>

static snd_status_t WINAPI win_open_process(DWORD pid, DWORD desired_access, HANDLE *out_process) {
    if (!out_process)
        return SND_ERR(SND_STATUS_NULL_POINTER);
    *out_process = OpenProcess(desired_access, FALSE, pid);
    return *out_process ? SND_OK : SND_ERR_W32(SND_STATUS_PROCESS_OPEN_FAILED);
}

static snd_status_t WINAPI win_alloc_remote(HANDLE process, SIZE_T size, DWORD allocation_type, DWORD protect,
                                            PVOID *out_address) {
    if (!out_address)
        return SND_ERR(SND_STATUS_NULL_POINTER);
    *out_address = VirtualAllocEx(process, NULL, size, allocation_type, protect);
    return *out_address ? SND_OK : SND_ERR_W32(SND_STATUS_VIRTUAL_ALLOC_FAILED);
}

static snd_status_t WINAPI win_write_remote(HANDLE process, PVOID base_address, const void *buffer, SIZE_T size,
                                            SIZE_T *bytes_written) {
    SIZE_T written = 0;
    BOOL   ok      = WriteProcessMemory(process, base_address, buffer, size, &written);
    if (bytes_written)
        *bytes_written = written;
    return ok ? SND_OK : SND_ERR_W32(SND_STATUS_VIRTUAL_WRITE_FAILED);
}

static snd_status_t WINAPI win_protect_remote(HANDLE process, PVOID base_address, SIZE_T size, DWORD new_protect,
                                              DWORD *old_protect) {
    DWORD old = 0;
    BOOL  ok  = VirtualProtectEx(process, base_address, size, new_protect, &old);
    if (old_protect)
        *old_protect = old;
    return ok ? SND_OK : SND_ERR_W32(SND_STATUS_VIRTUAL_PROTECT_FAILED);
}

static snd_status_t WINAPI win_create_remote_thread(HANDLE process, PVOID start_address, PVOID parameter,
                                                    HANDLE *out_thread) {
    if (!out_thread)
        return SND_ERR(SND_STATUS_NULL_POINTER);
    *out_thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)start_address, parameter, 0, NULL);
    return *out_thread ? SND_OK : SND_ERR_W32(SND_STATUS_THREAD_CREATE_FAILED);
}

static snd_status_t WINAPI win_close_handle(HANDLE handle) {
    return CloseHandle(handle) ? SND_OK : SND_ERR(SND_STATUS_HANDLE_CLOSE_FAILED);
}

const snd_process_api_t snd_proc_win = {.open_process         = win_open_process,
                                        .alloc_remote         = win_alloc_remote,
                                        .write_remote         = win_write_remote,
                                        .protect_remote       = win_protect_remote,
                                        .create_remote_thread = win_create_remote_thread,
                                        .close_handle         = win_close_handle};
