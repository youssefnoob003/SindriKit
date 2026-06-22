#include <sindri/common/buffer.h>
#include <sindri/common/disk.h>
#include <sindri/common/status.h>
#include <windows.h>

snd_status_t snd_buffer_load_from_disk(const char *path, snd_buffer_t *out_buf) {
    if (path == NULL) {
        return SND_ERR(SND_STATUS_INVALID_PATH);
    }

    if (out_buf == NULL) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }

    snd_buffer_init(out_buf, NULL, 0, NULL);

    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        return SND_ERR_W32_CTX(SND_STATUS_FILE_CREATE_FAILED, "Target file: %s", path);
    }

    LARGE_INTEGER fs;
    snd_status_t  status    = SND_OK;
    LPVOID        heap_data = NULL;

    if (!GetFileSizeEx(hFile, &fs)) {
        status = SND_ERR_W32_CTX(SND_STATUS_FILE_SIZE_QUERY_FAILED, "Target file: %s", path);
        goto cleanup;
    }

    if (fs.QuadPart == 0) {
        status = SND_ERR_CTX(SND_STATUS_FILE_TOO_SMALL, "Target file: %s", path);
        goto cleanup;
    }

    if (fs.QuadPart > MAXDWORD) {
        status = SND_ERR_CTX(SND_STATUS_FILE_TOO_LARGE, "Target file: %s", path);
        goto cleanup;
    }

    SIZE_T file_size = (SIZE_T)fs.QuadPart;

    heap_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, file_size);
    if (heap_data == NULL) {
        status = SND_ERR_W32_CTX(SND_STATUS_ALLOC_FAILED, "Target file: %s", path);
        goto cleanup;
    }

    DWORD bytes_read = 0;
    if (!ReadFile(hFile, heap_data, (DWORD)file_size, &bytes_read, NULL) || bytes_read != file_size) {
        status = SND_ERR_W32_CTX(SND_STATUS_FILE_READ_FAILED, "Target file: %s", path);
        goto cleanup;
    }

    snd_buffer_init(out_buf, heap_data, file_size, snd_buffer_free_heap);
    heap_data = NULL;

cleanup:
    if (heap_data != NULL) {
        HeapFree(GetProcessHeap(), 0, heap_data);
    }
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
    return status;
}
