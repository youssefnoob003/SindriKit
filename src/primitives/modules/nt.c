#include <sindri/common/status.h>
#include <sindri/common/string.h>
#include <sindri/internal/nt/api.h>
#include <sindri/parsers/env/peb.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/primitives/modules.h>
#include <sindri_hashes.h>
#include <stddef.h>
#include <windows.h>

static snd_status_t WINAPI nt_load_library(const char *module_name, HMODULE *out_module) {
    if (!out_module)
        return SND_ERR(SND_STATUS_NULL_POINTER);
    *out_module = NULL;

    if (!module_name)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    size_t  len = snd_strnlen(module_name, MAX_PATH - 1);
    wchar_t wname[MAX_PATH];
    snd_ascii_to_wide(wname, MAX_PATH, module_name, len);

    PVOID        base   = NULL;
    snd_status_t status = snd_peb_get_module_base(wname, &base);
    if (SND_SUCCEEDED(status)) {
        *out_module = (HMODULE)base;
        return SND_OK;
    }

    PVOID ntdll;
    status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC ldr_addr = NULL;
    status           = snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_LDRLOADDLL, &ldr_addr,
                                                      snd_peb_get_module_base);

    if (SND_FAILED(status))
        return status;

    SND_LdrLoadDll_t pfnLdrLoadDll = (SND_LdrLoadDll_t)ldr_addr;

    SND_UNICODE_STRING us;
    snd_init_unicode_string(&us, wname, len);

    HANDLE   hModule   = NULL;
    NTSTATUS nt_status = pfnLdrLoadDll(NULL, NULL, &us, &hModule);

    if (!SND_NT_SUCCESS(nt_status))
        return SND_ERR_NT(SND_STATUS_IMPORT_DLL_LOAD_FAILED, nt_status);

    *out_module = (HMODULE)hModule;
    return SND_OK;
}

static snd_status_t WINAPI nt_get_proc_address(HMODULE hModule, const char *proc_name, FARPROC *out_proc) {
    if (!out_proc)
        return SND_ERR(SND_STATUS_NULL_POINTER);
    *out_proc = NULL;
    if (!hModule || !proc_name)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    return snd_pe_get_export_address((PVOID)hModule, SND_SYS_DLL_SIZE_DEFAULT, proc_name, out_proc,
                                     snd_peb_get_module_base);
}

static snd_status_t WINAPI nt_get_proc_address_hash(HMODULE hModule, DWORD proc_hash, FARPROC *out_proc) {
    if (!out_proc)
        return SND_ERR(SND_STATUS_NULL_POINTER);
    *out_proc = NULL;
    if (!hModule || !proc_hash)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    return snd_pe_get_export_address_hash((PVOID)hModule, SND_SYS_DLL_SIZE_DEFAULT, proc_hash, out_proc,
                                          snd_peb_get_module_base);
}

const snd_module_api_t snd_mod_nt = {.load_library     = nt_load_library,
                                     .get_proc_address = nt_get_proc_address,
                                     .get_module_base  = snd_peb_get_module_base,

                                     .get_proc_address_hash = nt_get_proc_address_hash,
                                     .get_module_base_hash  = snd_peb_get_module_base_hash};
