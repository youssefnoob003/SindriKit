#include <sindri/common/string.h>
#include <sindri/internal/nt/api.h>
#include <sindri/internal/nt/file.h>
#include <sindri/parsers/env/ntdll.h>
#include <sindri/primitives/files.h>
#include <sindri/primitives/memory.h>
#include <sindri/primitives/status.h>
#include <sindri_hashes.h>

static snd_status_t nt_resolve(DWORD hash, FARPROC *address) {
    SND_CHECK_NULL(address);
    *address = NULL;
    return snd_ntdll_get_active_export(hash, address);
}

static snd_status_t nt_close_file(HANDLE file) {
    if (file == NULL || file == INVALID_HANDLE_VALUE) {
        return SND_OK;
    }

    FARPROC address = NULL;
    SND_TRY(nt_resolve(SND_HASH_NTCLOSE, &address));

    NTSTATUS nt_status = ((SND_NtClose_t)address)(file);
    if (SND_NT_FAILURE(nt_status)) {
        return SND_ERR_NT(SND_STATUS_HANDLE_CLOSE_FAILED, nt_status);
    }

    return SND_OK;
}

static snd_status_t nt_build_path(const char *path, wchar_t *wide_path, SIZE_T wide_path_count) {
    SND_CHECK_NULL(path, wide_path);

    if (wide_path_count <= 4) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETERS_COMBINATION);
    }

    size_t path_len = snd_strnlen(path, wide_path_count - 4);
    snd_ascii_to_wide(wide_path + 4, wide_path_count - 4, path, path_len);
    wide_path[0] = L'\\';
    wide_path[1] = L'?';
    wide_path[2] = L'?';
    wide_path[3] = L'\\';
    return SND_OK;
}

static snd_status_t WINAPI nt_load(const char *path, snd_buffer_t *out_buf) {
    SND_CHECK_NULL(path, out_buf);

    wchar_t wide_path[SND_MAX_PATH] = {0};
    SND_TRY(nt_build_path(path, wide_path, SND_MAX_PATH));

    SND_UNICODE_STRING name = {0};
    snd_init_unicode_string(&name, wide_path, snd_wcsnlen(wide_path, SND_MAX_PATH));

    SND_OBJECT_ATTRIBUTES attributes = {0};
    SND_InitializeObjectAttributes(&attributes, &name, SND_OBJ_CASE_INSENSITIVE, NULL, NULL);

    FARPROC create_address = NULL;
    SND_TRY(nt_resolve(SND_HASH_NTCREATEFILE, &create_address));

    HANDLE              file      = NULL;
    SND_IO_STATUS_BLOCK io        = {0};
    NTSTATUS            nt_status = ((SND_NtCreateFile_t)create_address)(
        &file, SND_GENERIC_READ | SND_SYNCHRONIZE, &attributes, &io, NULL, 0, SND_FILE_SHARE_READ, SND_FILE_OPEN,
        SND_FILE_NON_DIRECTORY_FILE | SND_FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    if (SND_NT_FAILURE(nt_status)) {
        return SND_ERR_NT(SND_STATUS_FILE_CREATE_FAILED, nt_status);
    }

    FARPROC      query_address = NULL;
    snd_status_t status        = nt_resolve(SND_HASH_NTQUERYINFORMATIONFILE, &query_address);
    if (SND_FAILED(status)) {
        (void)nt_close_file(file);
        return status;
    }

    SND_FILE_STANDARD_INFORMATION info = {0};
    nt_status = ((SND_NtQueryInformationFile_t)query_address)(file, &io, &info, sizeof(info),
                                                              SND_FILE_STANDARD_INFORMATION_CLASS);
    if (SND_NT_FAILURE(nt_status) || info.EndOfFile.QuadPart <= 0 ||
        (ULONGLONG)info.EndOfFile.QuadPart > SND_MAX_DWORD) {
        (void)nt_close_file(file);
        return SND_ERR_NT(SND_STATUS_FILE_SIZE_QUERY_FAILED, nt_status);
    }

    SIZE_T size = (SIZE_T)info.EndOfFile.QuadPart;
    PVOID  data = NULL;
    status      = snd_mem_nt.alloc(NULL, size, SND_MEM_COMMIT | SND_MEM_RESERVE, SND_PAGE_READWRITE, &data);
    if (SND_FAILED(status)) {
        (void)nt_close_file(file);
        return status;
    }

    FARPROC read_address = NULL;
    status               = nt_resolve(SND_HASH_NTREADFILE, &read_address);
    if (SND_FAILED(status)) {
        (void)snd_mem_nt.free(data, 0, SND_MEM_RELEASE);
        (void)nt_close_file(file);
        return status;
    }

    nt_status = ((SND_NtReadFile_t)read_address)(file, NULL, NULL, NULL, &io, data, (ULONG)size, NULL, NULL);

    snd_status_t close_status = nt_close_file(file);
    file                      = NULL;
    if (SND_FAILED(close_status)) {
        (void)snd_mem_nt.free(data, 0, SND_MEM_RELEASE);
        return close_status;
    }
    if (SND_NT_FAILURE(nt_status) || io.Information != size) {
        (void)snd_mem_nt.free(data, 0, SND_MEM_RELEASE);
        return SND_ERR_NT(SND_STATUS_FILE_READ_FAILED, nt_status);
    }

    snd_buffer_init(out_buf, data, size, snd_buffer_free_nt);
    return SND_OK;
}

const snd_file_api_t snd_file_nt = {.load = nt_load};
