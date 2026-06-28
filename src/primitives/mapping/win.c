#include <sindri/common/status.h>
#include <sindri/primitives/mapping.h>
#include <sindri/primitives/os_api.h>
#include <windows.h>

/*
 * NOTE: This function relies on OpenFileMappingW and is subject to the Win32 sandbox.
 * It only accepts standard DOS paths (e.g., "C:\file.ext") or Win32 namespaces
 * (e.g., "Local\MappingName").
 *
 * Do NOT use this function for absolute NT Object Manager paths (e.g., "\KnownDlls\...").
 * The Win32 subsystem will prepend its namespace, corrupting the path, and it will
 * fail with OS Error 161 (ERROR_BAD_PATHNAME). Use the Nt or sys API instead.
 */
static snd_status_t WINAPI win_mapping_open(const wchar_t *section_name, HANDLE *out_handle) {
    if (!out_handle || !section_name)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    HANDLE handle = OpenFileMappingW(FILE_MAP_READ, FALSE, section_name);
    if (!handle)
        return SND_ERR_W32(SND_STATUS_SECTION_OPEN_FAILED);

    *out_handle = handle;
    return SND_OK;
}

static snd_status_t WINAPI win_mapping_view(HANDLE section_handle, PVOID *out_base, SIZE_T *out_size) {
    if (!section_handle || !out_base || !out_size)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    PVOID base = MapViewOfFile(section_handle, FILE_MAP_READ, 0, 0, 0);
    if (!base)
        return SND_ERR_W32(SND_STATUS_SECTION_MAP_FAILED);

    MEMORY_BASIC_INFORMATION mbi = {0};
    if (VirtualQuery(base, &mbi, sizeof(mbi)) == 0) {
        UnmapViewOfFile(base);
        return SND_ERR_W32(SND_STATUS_SECTION_MAP_FAILED);
    }

    *out_base = base;
    *out_size = mbi.RegionSize;
    return SND_OK;
}

static snd_status_t WINAPI win_mapping_close(HANDLE handle) {
    if (!handle)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    if (!CloseHandle(handle))
        return SND_ERR_W32(SND_STATUS_HANDLE_CLOSE_FAILED);

    return SND_OK;
}

const snd_mapping_api_t snd_map_win = {.open = win_mapping_open, .view = win_mapping_view, .close = win_mapping_close};
