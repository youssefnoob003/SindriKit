#include <sindri/common/helpers.h>
#include <sindri/common/status.h>
#include <sindri/internal/nt_defs.h>
#include <sindri/parsers/pe/pe_exports.h>
#include <sindri/parsers/pe/pe_parser.h>
#include <sindri/primitives/modules.h>
#include <sindri/primitives/ntdll.h>
#include <sindri/primitives/peb.h>
#include <sindri_hashes.h>
#include <stddef.h>
#include <windows.h>

static void build_unicode_string(SND_UNICODE_STRING *us, wchar_t *buf, USHORT char_count) {
    us->Buffer        = buf;
    us->Length        = (USHORT)(char_count * sizeof(wchar_t));
    us->MaximumLength = us->Length + sizeof(wchar_t);
}

static snd_status_t WINAPI native_load_library(const char *module_name, HMODULE *out_module) {
    if (!out_module)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    *out_module = NULL;
    if (!module_name)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);

    wchar_t wname[MAX_PATH];
    size_t  len = snd_strnlen(module_name, MAX_PATH - 1);
    for (size_t i = 0; i <= len; i++) {
        wname[i] = (wchar_t)(unsigned char)module_name[i];
    }

    PVOID base = NULL;
    snd_peb_get_module_base(wname, &base);
    if (base) {
        *out_module = (HMODULE)base;
        return SND_OK;
    }

    PVOID ntdll = snd_get_ntdll();
    if (!ntdll)
        return SND_ERR_CTX(SND_STATUS_NOT_INITIALIZED, "NTDLL base is NULL");

    FARPROC ldr_addr = NULL;
    snd_pe_get_export_address_by_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_LDRLOADDLL, &ldr_addr,
                                      snd_peb_get_module_base_by_hash);
    if (!ldr_addr)
        return SND_ERR(SND_STATUS_EXPORT_NOT_FOUND);

    LdrLoadDll_t pfnLdrLoadDll = (LdrLoadDll_t)ldr_addr;

    SND_UNICODE_STRING us;
    build_unicode_string(&us, wname, (USHORT)len);

    HANDLE   hModule = NULL;
    NTSTATUS status  = pfnLdrLoadDll(NULL, NULL, &us, &hModule);
    if (!NT_SUCCESS(status) || !hModule)
        return SND_ERR(SND_STATUS_IMPORT_DLL_LOAD_FAILED);
    *out_module = (HMODULE)hModule;
    return SND_OK;
}

static snd_status_t WINAPI native_get_proc_address(HMODULE hModule, const char *proc_name, FARPROC *out_proc) {
    if (!out_proc)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    *out_proc = NULL;
    if (!hModule || !proc_name)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);

    return snd_pe_get_export_address((PVOID)hModule, SND_SYS_DLL_SIZE_DEFAULT, proc_name, out_proc,
                                     snd_peb_get_module_base);
}

static snd_status_t WINAPI native_load_library_hash(DWORD module_hash, HMODULE *out_module) {
    if (!out_module)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    *out_module = NULL;
    PVOID base  = NULL;
    snd_peb_get_module_base_by_hash(module_hash, &base);
    *out_module = (HMODULE)base;
    return *out_module ? SND_OK : SND_ERR(SND_ERROR_GENERIC);
}

static snd_status_t WINAPI native_get_proc_address_hash(HMODULE hModule, DWORD proc_hash, FARPROC *out_proc) {
    if (!out_proc)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    *out_proc = NULL;
    if (!hModule || !proc_hash)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);

    return snd_pe_get_export_address_by_hash((PVOID)hModule, SND_SYS_DLL_SIZE_DEFAULT, proc_hash, out_proc,
                                             snd_peb_get_module_base_by_hash);
}

static snd_status_t WINAPI native_get_module_base_hash(DWORD module_hash, PVOID *out_base) {
    return snd_peb_get_module_base_by_hash(module_hash, out_base);
}

const snd_module_api_t snd_mod_native = {.load_library     = native_load_library,
                                         .get_proc_address = native_get_proc_address,
                                         .get_module_base  = snd_peb_get_module_base,

                                         .load_library_hash     = native_load_library_hash,
                                         .get_proc_address_hash = native_get_proc_address_hash,
                                         .get_module_base_hash  = native_get_module_base_hash};
