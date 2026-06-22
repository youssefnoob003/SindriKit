#include <sindri/internal/nt_defs.h>
#include <sindri/loaders/knowndlls/knowndlls.h>
#include <windows.h>

#ifndef SECTION_MAP_READ
#define SECTION_MAP_READ 0x0004
#endif
#ifndef SECTION_QUERY
#define SECTION_QUERY 0x0001
#endif
#ifndef ViewShare
#define ViewShare 1
#endif

snd_status_t snd_map_knowndll(const snd_knowndlls_config_t *config, const wchar_t *dll_name, PVOID *out_base_address) {
    if (!config || !out_base_address || !dll_name)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);

    *out_base_address = NULL;

    if (!config->open_section || !config->map_view_of_section) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }

    wchar_t full_path[MAX_PATH];
    full_path[0] = L'\0';

    const wchar_t *prefix = SND_TARGET_KNOWNDLLS_DIR;
    int            i      = 0;
    while (prefix[i] != L'\0' && i < MAX_PATH - 1) {
        full_path[i] = prefix[i];
        i++;
    }

    int j = 0;
    while (dll_name[j] != L'\0' && i < MAX_PATH - 1) {
        full_path[i] = dll_name[j];
        i++;
        j++;
    }
    full_path[i] = L'\0';

    SND_UNICODE_STRING us;
    us.Buffer        = full_path;
    us.Length        = (USHORT)(i * sizeof(wchar_t));
    us.MaximumLength = us.Length + sizeof(wchar_t);

    SND_OBJECT_ATTRIBUTES oa;
    SND_InitializeObjectAttributes(&oa, &us, SND_OBJ_CASE_INSENSITIVE, NULL, NULL);

    HANDLE       hSection = NULL;
    snd_status_t status   = config->open_section(&hSection, SECTION_MAP_READ | SECTION_QUERY, &oa);

    if (status.code != SND_SUCCESS || !hSection) {
        return status;
    }

    PVOID  base_addr = NULL;
    SIZE_T view_size = 0;

    status = config->map_view_of_section(hSection, (HANDLE)-1, &base_addr, 0, 0, NULL, &view_size, ViewShare, 0,
                                         PAGE_READONLY);

    if (config->close_handle) {
        config->close_handle(hSection);
    }

    if (status.code != SND_SUCCESS || !base_addr) {
        return status;
    }

    *out_base_address = base_addr;
    return SND_OK;
}
