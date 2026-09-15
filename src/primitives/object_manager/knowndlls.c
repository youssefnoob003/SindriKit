#include <sindri/common/debug.h>
#include <sindri/common/string.h>
#include <sindri/internal/windows/types.h>
#include <sindri/primitives/object_manager.h>
#include <sindri/primitives/os_api.h>
#include <sindri/primitives/status.h>

snd_status_t snd_om_knowndll_map(const snd_mapping_api_t *config, const wchar_t *dll_name, PVOID *out_base_address) {
    SND_CHECK_NULL(config, out_base_address, dll_name);

    *out_base_address = NULL;

    if (!config->open || !config->view) {
        return SND_ERR(SND_STATUS_OM_NOT_INITIALIZED);
    }

    wchar_t full_path[SND_MAX_PATH];
    snd_wcsncpy(full_path, SND_MAX_PATH, SND_TARGET_KNOWNDLLS_DIR, SND_MAX_PATH);
    snd_wcsncat(full_path, SND_MAX_PATH, dll_name, SND_MAX_PATH);

    HANDLE hSection = NULL;
    SND_TRY(config->open(full_path, &hSection));

    PVOID        base_addr = NULL;
    SIZE_T       view_size = 0;
    snd_status_t status    = config->view(hSection, &base_addr, &view_size);

    if (config->close && hSection) {
        snd_status_t close_status = config->close(hSection);
        if (SND_FAILED(close_status)) {
            SND_DEBUG_PRINT("[!] Failed to close section handle %p: 0x%08X", hSection, close_status.code);
        }
    }

    if (SND_FAILED(status)) {
        return status;
    }

    *out_base_address = base_addr;
    return SND_OK;
}
