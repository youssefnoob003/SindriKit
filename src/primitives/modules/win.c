#include <sindri/primitives/modules.h>
#include <sindri/primitives/status.h>
#include <windows.h>

static snd_status_t WINAPI load_import_library(const char *dll_name, HMODULE *out_module) {
    SND_CHECK_NULL(dll_name, out_module);

    DWORD load_flags = 0;
#ifdef LOAD_LIBRARY_SEARCH_SYSTEM32
    load_flags |= LOAD_LIBRARY_SEARCH_SYSTEM32;
#endif

    *out_module = load_flags ? LoadLibraryExA(dll_name, NULL, load_flags) : LoadLibraryA(dll_name);

    return *out_module ? SND_OK : SND_ERR_W32(SND_STATUS_MODULE_LOAD_FAILED);
}

static snd_status_t WINAPI win_get_proc_address(HMODULE hModule, LPCSTR lpProcName, FARPROC *out_proc) {
    SND_CHECK_NULL(hModule, lpProcName, out_proc);

    *out_proc = GetProcAddress(hModule, lpProcName);
    return *out_proc ? SND_OK : SND_ERR_W32(SND_STATUS_PROC_RESOLVE_FAILED);
}

static snd_status_t WINAPI win_get_module_base(const wchar_t *module_name, PVOID *out_base) {
    SND_CHECK_NULL(out_base);

    *out_base = (PVOID)GetModuleHandleW(module_name);
    return *out_base ? SND_OK : SND_ERR_W32(SND_STATUS_MODULE_BASE_GET_FAILED);
}

const snd_module_api_t snd_mod_win = {.load_library     = load_import_library,
                                      .get_proc_address = win_get_proc_address,
                                      .get_module_base  = win_get_module_base};
