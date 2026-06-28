#include <sindri/common/status.h>
#include <sindri/primitives/modules.h>
#include <windows.h>

static snd_status_t WINAPI load_import_library(const char *dll_name, HMODULE *out_module) {
    if (!out_module)
        return SND_ERR(SND_STATUS_NULL_POINTER);
    DWORD load_flags = 0;
#ifdef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
    load_flags |= LOAD_LIBRARY_SEARCH_DEFAULT_DIRS;
#endif
#ifdef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
    load_flags |= LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR;
#endif

    if (load_flags != 0) {
        *out_module = LoadLibraryExA(dll_name, NULL, load_flags);
    } else {
        *out_module = LoadLibraryA(dll_name);
    }
    return *out_module ? SND_OK : SND_ERR_W32(SND_STATUS_IMPORT_DLL_LOAD_FAILED);
}

static snd_status_t WINAPI win_get_proc_address(HMODULE hModule, LPCSTR lpProcName, FARPROC *out_proc) {
    if (!out_proc)
        return SND_ERR(SND_STATUS_NULL_POINTER);
    *out_proc = GetProcAddress(hModule, lpProcName);
    return *out_proc ? SND_OK : SND_ERR_W32(SND_STATUS_IMPORT_SYMBOL_RESOLVE_FAILED);
}

static snd_status_t WINAPI win_get_module_base(const wchar_t *module_name, PVOID *out_base) {
    if (!out_base)
        return SND_ERR(SND_STATUS_NULL_POINTER);
    *out_base = (PVOID)GetModuleHandleW(module_name);
    return *out_base ? SND_OK : SND_ERR_W32(SND_STATUS_MODULE_NOT_FOUND);
}

const snd_module_api_t snd_mod_win = {.load_library     = load_import_library,
                                      .get_proc_address = win_get_proc_address,
                                      .get_module_base  = win_get_module_base};
