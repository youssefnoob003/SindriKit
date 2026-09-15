#include <sindri/internal/win32.h>
#include <sindri/primitives/files.h>
#include <sindri/primitives/memory.h>
#include <sindri/primitives/status.h>
#include <windows.h>

static snd_status_t WINAPI win_load(const char *path, snd_buffer_t *out_buf) {
    SND_CHECK_NULL(path, out_buf);

    HANDLE file = CreateFileA(path, SND_GENERIC_READ, SND_FILE_SHARE_READ, NULL, SND_OPEN_EXISTING,
                              SND_FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return SND_ERR_W32_CTX(SND_STATUS_FILE_CREATE_FAILED, "Target file: %s", path);

    LARGE_INTEGER file_size = {0};
    snd_status_t  status    = SND_OK;
    PVOID         data      = NULL;

    if (!GetFileSizeEx(file, &file_size)) {
        status = SND_ERR_W32_CTX(SND_STATUS_FILE_SIZE_QUERY_FAILED, "Target file: %s", path);
        goto cleanup;
    }
    if (file_size.QuadPart <= 0) {
        status = SND_ERR_CTX(SND_STATUS_FILE_TOO_SMALL, "Target file: %s", path);
        goto cleanup;
    }
    if ((ULONGLONG)file_size.QuadPart > SND_MAX_DWORD) {
        status = SND_ERR_CTX(SND_STATUS_FILE_TOO_LARGE, "Target file: %s", path);
        goto cleanup;
    }

    DWORD size = (DWORD)file_size.QuadPart;
    data       = HeapAlloc(GetProcessHeap(), SND_HEAP_ZERO_MEMORY, size);
    if (!data) {
        status = SND_ERR_W32_CTX(SND_STATUS_FILE_ALLOC_FAILED, "Target file: %s", path);
        goto cleanup;
    }

    DWORD read = 0;
    if (!ReadFile(file, data, size, &read, NULL) || read != size) {
        status = SND_ERR_W32_CTX(SND_STATUS_FILE_READ_FAILED, "Target file: %s", path);
        goto cleanup;
    }

    snd_buffer_init(out_buf, data, size, snd_buffer_free_win);
    data = NULL;

cleanup:
    if (data)
        HeapFree(GetProcessHeap(), 0, data);
    CloseHandle(file);
    return status;
}

const snd_file_api_t snd_file_win = {.load = win_load};
