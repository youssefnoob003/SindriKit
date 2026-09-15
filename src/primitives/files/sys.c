#include <sindri/common/string.h>
#include <sindri/internal/nt/file.h>
#include <sindri/primitives/files.h>
#include <sindri/primitives/memory.h>
#include <sindri/primitives/status.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>

static snd_status_t sys_close_file(HANDLE file) {
    if (file == NULL || file == INVALID_HANDLE_VALUE) {
        return SND_OK;
    }

    snd_syscall_args_t args      = {.arg1 = file};
    NTSTATUS           nt_status = 0;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTCLOSE, &args, &nt_status));

    if (SND_NT_FAILURE(nt_status)) {
        return SND_ERR_NT(SND_STATUS_HANDLE_CLOSE_FAILED, nt_status);
    }

    return SND_OK;
}

static snd_status_t sys_build_path(const char *path, wchar_t *wide_path, SIZE_T wide_path_count) {
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

static snd_status_t WINAPI sys_load(const char *path, snd_buffer_t *out_buf) {
    SND_CHECK_NULL(path, out_buf);

    wchar_t wide_path[SND_MAX_PATH] = {0};
    SND_TRY(sys_build_path(path, wide_path, SND_MAX_PATH));

    SND_UNICODE_STRING name = {0};
    snd_init_unicode_string(&name, wide_path, snd_wcsnlen(wide_path, SND_MAX_PATH));

    SND_OBJECT_ATTRIBUTES attributes = {0};
    SND_InitializeObjectAttributes(&attributes, &name, SND_OBJ_CASE_INSENSITIVE, NULL, NULL);

    HANDLE              file = NULL;
    SND_IO_STATUS_BLOCK io   = {0};
    snd_syscall_args_t  args = {
        .arg1 = &file,
        .arg2 = (PVOID)(ULONG_PTR)(SND_GENERIC_READ | SND_SYNCHRONIZE),
        .arg3 = &attributes,
        .arg4 = &io,
        .arg7 = (PVOID)(ULONG_PTR)SND_FILE_SHARE_READ,
        .arg8 = (PVOID)(ULONG_PTR)SND_FILE_OPEN,
        .arg9 = (PVOID)(ULONG_PTR)(SND_FILE_NON_DIRECTORY_FILE | SND_FILE_SYNCHRONOUS_IO_NONALERT),
    };

    NTSTATUS     nt_status = 0;
    snd_status_t status    = snd_syscall_invoke(SND_HASH_NTCREATEFILE, &args, &nt_status);
    if (SND_FAILED(status)) {
        return status;
    }
    if (SND_NT_FAILURE(nt_status)) {
        return SND_ERR_NT(SND_STATUS_FILE_CREATE_FAILED, nt_status);
    }

    SND_FILE_STANDARD_INFORMATION info = {0};
    args                               = (snd_syscall_args_t){
        .arg1 = file,
        .arg2 = &io,
        .arg3 = &info,
        .arg4 = (PVOID)(ULONG_PTR)sizeof(info),
        .arg5 = (PVOID)(ULONG_PTR)SND_FILE_STANDARD_INFORMATION_CLASS,
    };
    status = snd_syscall_invoke(SND_HASH_NTQUERYINFORMATIONFILE, &args, &nt_status);
    if (SND_FAILED(status)) {
        (void)sys_close_file(file);
        return status;
    }
    if (SND_NT_FAILURE(nt_status) || info.EndOfFile.QuadPart <= 0 ||
        (ULONGLONG)info.EndOfFile.QuadPart > SND_MAX_DWORD) {
        (void)sys_close_file(file);
        return SND_ERR_NT(SND_STATUS_FILE_SIZE_QUERY_FAILED, nt_status);
    }

    SIZE_T size = (SIZE_T)info.EndOfFile.QuadPart;
    PVOID  data = NULL;
    status      = snd_mem_sys.alloc(NULL, size, SND_MEM_COMMIT | SND_MEM_RESERVE, SND_PAGE_READWRITE, &data);
    if (SND_FAILED(status)) {
        (void)sys_close_file(file);
        return status;
    }

    args = (snd_syscall_args_t){
        .arg1 = file,
        .arg5 = &io,
        .arg6 = data,
        .arg7 = (PVOID)(ULONG_PTR)(ULONG)size,
    };
    status = snd_syscall_invoke(SND_HASH_NTREADFILE, &args, &nt_status);

    snd_status_t close_status = sys_close_file(file);
    file                      = NULL;
    if (SND_FAILED(status)) {
        (void)snd_mem_sys.free(data, 0, SND_MEM_RELEASE);
        return status;
    }
    if (SND_FAILED(close_status)) {
        (void)snd_mem_sys.free(data, 0, SND_MEM_RELEASE);
        return close_status;
    }
    if (SND_NT_FAILURE(nt_status) || io.Information != size) {
        (void)snd_mem_sys.free(data, 0, SND_MEM_RELEASE);
        return SND_ERR_NT(SND_STATUS_FILE_READ_FAILED, nt_status);
    }

    snd_buffer_init(out_buf, data, size, snd_buffer_free_sys);
    return SND_OK;
}

const snd_file_api_t snd_file_sys = {.load = sys_load};
