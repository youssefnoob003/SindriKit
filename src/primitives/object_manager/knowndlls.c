#include <sindri/common/status.h>
#include <sindri/common/string.h>
#include <sindri/primitives/object_manager.h>
#include <sindri/primitives/os_api.h>
#include <windows.h>

snd_status_t snd_om_knowndll_map(const snd_mapping_api_t *config, const wchar_t *dll_name, PVOID *out_base_address) {
    if (!config || !out_base_address || !dll_name)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    *out_base_address = NULL;

    if (!config->open || !config->view) {
        return SND_ERR(SND_STATUS_OM_NOT_INITIALIZED);
    }

    wchar_t full_path[MAX_PATH];

    snd_wcsncpy(full_path, MAX_PATH, SND_TARGET_KNOWNDLLS_DIR, MAX_PATH);
    snd_wcsncat(full_path, MAX_PATH, dll_name, MAX_PATH);

    HANDLE       hSection = NULL;
    snd_status_t status   = config->open(full_path, &hSection);

    if (SND_FAILED(status) || !hSection) {
        return status;
    }

    PVOID  base_addr = NULL;
    SIZE_T view_size = 0;

    status = config->view(hSection, &base_addr, &view_size);

    if (config->close) {
        config->close(hSection);
    }

    if (SND_FAILED(status) || !base_addr) {
        return status;
    }

    *out_base_address = base_addr;
    return SND_OK;
}
