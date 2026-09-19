#include <sindri/common/string.h>
#include <sindri/primitives/process.h>
#include <sindri/primitives/status.h>
#include <windows.h>

static snd_status_t WINAPI win_create_process_params(const void *nt_path, const wchar_t *cmd_line, PVOID *out_params) {
    (void)nt_path;
    (void)cmd_line;
    if (out_params) {
        *out_params = NULL;
    }
    return SND_OK;
}

static snd_status_t WINAPI win_free_process_params(PVOID params) {
    (void)params;
    return SND_OK;
}

static snd_status_t WINAPI win_create_process(const snd_process_api_t *api, const wchar_t *image_path,
                                              const wchar_t *command_line, HANDLE *out_process, HANDLE *out_thread) {
    (void)api;
    SND_CHECK_NULL(out_process, out_thread);

    *out_process = NULL;
    *out_thread  = NULL;

    wchar_t cmd_buffer[SND_MAX_PATH] = {0};
    if (command_line) {
        snd_wcsncpy(cmd_buffer, SND_MAX_PATH, command_line, SND_MAX_PATH);
    }

    STARTUPINFOW si        = {0};
    si.cb                  = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    BOOL ok = CreateProcessW(image_path, command_line ? cmd_buffer : NULL, NULL, NULL, FALSE,
                             CREATE_SUSPENDED | CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (!ok) {
        return SND_ERR_W32(SND_STATUS_PROCESS_CREATE_FAILED);
    }

    *out_process = pi.hProcess;
    *out_thread  = pi.hThread;

    return SND_OK;
}

static snd_status_t WINAPI win_open_process(DWORD pid, DWORD desired_access, HANDLE *out_process) {
    SND_CHECK_NULL(out_process);
    *out_process = NULL;

    *out_process = OpenProcess(desired_access, FALSE, pid);
    return *out_process ? SND_OK : SND_ERR_W32(SND_STATUS_PROCESS_OPEN_FAILED);
}

static snd_status_t WINAPI win_alloc_remote(HANDLE process, SIZE_T size, DWORD allocation_type, DWORD protect,
                                            PVOID *out_address) {
    SND_CHECK_NULL(process, out_address);
    *out_address = NULL;

    *out_address = VirtualAllocEx(process, NULL, size, allocation_type, protect);
    return *out_address ? SND_OK : SND_ERR_W32(SND_STATUS_PROCESS_REMOTE_ALLOC_FAILED);
}

static snd_status_t WINAPI win_write_remote(HANDLE process, PVOID base_address, const void *buffer, SIZE_T size,
                                            SIZE_T *bytes_written) {
    SND_CHECK_NULL(process, base_address, buffer);

    SIZE_T written = 0;
    BOOL   ok      = WriteProcessMemory(process, base_address, buffer, size, &written);
    if (bytes_written) {
        *bytes_written = written;
    }

    return ok ? SND_OK : SND_ERR_W32(SND_STATUS_PROCESS_REMOTE_WRITE_FAILED);
}

static snd_status_t WINAPI win_protect_remote(HANDLE process, PVOID base_address, SIZE_T size, DWORD new_protect,
                                              DWORD *old_protect) {
    SND_CHECK_NULL(process, base_address);

    DWORD old = 0;
    BOOL  ok  = VirtualProtectEx(process, base_address, size, new_protect, &old);
    if (old_protect) {
        *old_protect = old;
    }

    return ok ? SND_OK : SND_ERR_W32(SND_STATUS_PROCESS_REMOTE_PROTECT_FAILED);
}

static snd_status_t WINAPI win_create_remote_thread(HANDLE process, PVOID start_address, PVOID parameter,
                                                    HANDLE *out_thread) {
    SND_CHECK_NULL(out_thread, process, start_address);
    *out_thread = NULL;

    *out_thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)start_address, parameter, 0, NULL);
    return *out_thread ? SND_OK : SND_ERR_W32(SND_STATUS_THREAD_REMOTE_CREATE_FAILED);
}

static snd_status_t WINAPI win_close_handle(HANDLE handle) {
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        return SND_OK;
    }

    return CloseHandle(handle) ? SND_OK : SND_ERR_W32(SND_STATUS_HANDLE_CLOSE_FAILED);
}

const snd_process_api_t snd_proc_win = {.create_process_params = win_create_process_params,
                                        .free_process_params   = win_free_process_params,
                                        .create_process        = win_create_process,
                                        .open_process          = win_open_process,
                                        .alloc_remote          = win_alloc_remote,
                                        .write_remote          = win_write_remote,
                                        .protect_remote        = win_protect_remote,
                                        .create_remote_thread  = win_create_remote_thread,
                                        .close_handle          = win_close_handle};
