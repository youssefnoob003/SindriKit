#include <sindri/common/string.h>
#include <sindri/internal/nt/base.h>
#include <sindri/internal/nt/process.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/primitives/os_api.h>
#include <sindri/primitives/status.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>

static snd_status_t WINAPI sys_mapping_open(const wchar_t *section_name, HANDLE *out_handle) {
    SND_CHECK_NULL(out_handle, section_name);
    *out_handle = NULL;

    SND_UNICODE_STRING us_name;
    snd_init_unicode_string(&us_name, section_name, snd_wcsnlen(section_name, SND_UNICODE_STRING_MAX_CHARS));

    SND_OBJECT_ATTRIBUTES obj_attr;
    SND_InitializeObjectAttributes(&obj_attr, &us_name, SND_OBJ_CASE_INSENSITIVE, NULL, NULL);

    snd_syscall_args_t args = {.arg1 = out_handle, .arg2 = (PVOID)(ULONG_PTR)SND_SECTION_MAP_READ, .arg3 = &obj_attr};

    NTSTATUS nt_status = 0;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTOPENSECTION, &args, &nt_status));

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_MAPPING_OPEN_FAILED, nt_status);
}

static snd_status_t WINAPI sys_mapping_view(HANDLE section_handle, PVOID *out_base, SIZE_T *out_size) {
    SND_CHECK_NULL(section_handle, out_base, out_size);
    *out_base = NULL;
    *out_size = 0;

    PVOID             base_address   = NULL;
    SIZE_T            view_size      = 0;
    SND_LARGE_INTEGER section_offset = {0};

    snd_syscall_args_t args = {.arg1  = section_handle,
                               .arg2  = SND_CURRENT_PROCESS,
                               .arg3  = &base_address,
                               .arg4  = (PVOID)0,
                               .arg5  = (PVOID)0,
                               .arg6  = &section_offset,
                               .arg7  = &view_size,
                               .arg8  = (PVOID)(ULONG_PTR)1,
                               .arg9  = (PVOID)0,
                               .arg10 = (PVOID)(ULONG_PTR)SND_PAGE_READONLY};

    NTSTATUS nt_status = 0;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTMAPVIEWOFSECTION, &args, &nt_status));

    if (SND_NT_FAILURE(nt_status)) {
        return SND_ERR_NT(SND_STATUS_MAPPING_VIEW_FAILED, nt_status);
    }

    *out_base = base_address;
    *out_size = view_size;
    return SND_OK;
}

static snd_status_t WINAPI sys_mapping_close(HANDLE handle) {
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        return SND_OK;
    }

    snd_syscall_args_t args = {.arg1 = handle};

    NTSTATUS nt_status = 0;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTCLOSE, &args, &nt_status));

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_MAPPING_HANDLE_CLOSE_FAILED, nt_status);
}

const snd_mapping_api_t snd_map_sys = {.open = sys_mapping_open, .view = sys_mapping_view, .close = sys_mapping_close};
