#include <sindri/common/status.h>
#include <sindri/internal/nt/api.h>
#include <sindri/internal/nt/types.h>
#include <sindri/parsers/env/peb.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/primitives/mapping.h>
#include <sindri/primitives/os_api.h>
#include <sindri_hashes.h>
#include <windows.h>

static snd_status_t WINAPI nt_mapping_open(const wchar_t *section_name, HANDLE *out_handle) {
    if (!out_handle || !section_name)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status = snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTOPENSECTION, &func_addr, NULL);
    if (SND_FAILED(status))
        return status;

    SND_NtOpenSection_t pNtOpenSection = (SND_NtOpenSection_t)func_addr;

    SND_UNICODE_STRING us_name;
    snd_init_unicode_string(&us_name, section_name, wcslen(section_name));

    SND_OBJECT_ATTRIBUTES obj_attr;
    SND_InitializeObjectAttributes(&obj_attr, &us_name, SND_OBJ_CASE_INSENSITIVE, NULL, NULL);

    NTSTATUS nt_status = pNtOpenSection(out_handle, SECTION_MAP_READ, &obj_attr);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_SECTION_OPEN_FAILED, nt_status);
}

static snd_status_t WINAPI nt_mapping_view(HANDLE section_handle, PVOID *out_base, SIZE_T *out_size) {
    if (!section_handle || !out_base || !out_size)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status =
        snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTMAPVIEWOFSECTION, &func_addr, NULL);
    if (SND_FAILED(status))
        return status;

    SND_NtMapViewOfSection_t pNtMapViewOfSection = (SND_NtMapViewOfSection_t)func_addr;

    LARGE_INTEGER section_offset = {0};

    NTSTATUS nt_status = pNtMapViewOfSection(section_handle, GetCurrentProcess(), out_base, 0, 0, &section_offset,
                                             out_size, 1, 0, PAGE_READONLY);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_SECTION_MAP_FAILED, nt_status);
}

static snd_status_t WINAPI nt_mapping_close(HANDLE handle) {
    if (!handle)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status = snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTCLOSE, &func_addr, NULL);
    if (SND_FAILED(status))
        return status;

    SND_NtClose_t pNtClose = (SND_NtClose_t)func_addr;

    NTSTATUS nt_status = pNtClose(handle);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_HANDLE_CLOSE_FAILED, nt_status);
}

const snd_mapping_api_t snd_map_nt = {.open = nt_mapping_open, .view = nt_mapping_view, .close = nt_mapping_close};
