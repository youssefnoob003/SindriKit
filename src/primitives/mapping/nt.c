#include <sindri/common/string.h>
#include <sindri/internal/nt/api.h>
#include <sindri/internal/nt/base.h>
#include <sindri/internal/nt/process.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/parsers/env/ntdll.h>
#include <sindri/primitives/mapping.h>
#include <sindri/primitives/os_api.h>
#include <sindri/primitives/status.h>
#include <sindri_hashes.h>

static snd_status_t WINAPI nt_mapping_open(const wchar_t *section_name, HANDLE *out_handle) {
    SND_CHECK_NULL(out_handle, section_name);

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTOPENSECTION, &func_addr));

    SND_NtOpenSection_t pNtOpenSection = (SND_NtOpenSection_t)func_addr;

    SND_UNICODE_STRING us_name;
    snd_init_unicode_string(&us_name, section_name, snd_wcsnlen(section_name, SND_UNICODE_STRING_MAX_CHARS));

    SND_OBJECT_ATTRIBUTES obj_attr;
    SND_InitializeObjectAttributes(&obj_attr, &us_name, SND_OBJ_CASE_INSENSITIVE, NULL, NULL);

    NTSTATUS nt_status = pNtOpenSection(out_handle, SND_SECTION_MAP_READ, &obj_attr);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_MAPPING_OPEN_FAILED, nt_status);
}

static snd_status_t WINAPI nt_mapping_view(HANDLE section_handle, PVOID *out_base, SIZE_T *out_size) {
    SND_CHECK_NULL(section_handle, out_base, out_size);

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTMAPVIEWOFSECTION, &func_addr));

    SND_NtMapViewOfSection_t pNtMapViewOfSection = (SND_NtMapViewOfSection_t)func_addr;
    SND_LARGE_INTEGER        section_offset      = {0};
    NTSTATUS nt_status = pNtMapViewOfSection(section_handle, SND_CURRENT_PROCESS, out_base, 0, 0, &section_offset,
                                             out_size, 1, 0, SND_PAGE_READONLY);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_MAPPING_VIEW_FAILED, nt_status);
}

static snd_status_t WINAPI nt_mapping_close(HANDLE handle) {
    SND_CHECK_NULL(handle);

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTCLOSE, &func_addr));

    SND_NtClose_t pNtClose  = (SND_NtClose_t)func_addr;
    NTSTATUS      nt_status = pNtClose(handle);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_MAPPING_HANDLE_CLOSE_FAILED, nt_status);
}

const snd_mapping_api_t snd_map_nt = {.open = nt_mapping_open, .view = nt_mapping_view, .close = nt_mapping_close};
