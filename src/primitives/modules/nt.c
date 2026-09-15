#include <sindri/common/string.h>
#include <sindri/internal/nt/api.h>
#include <sindri/internal/windows/types.h>
#include <sindri/parsers/env/ntdll.h>
#include <sindri/parsers/env/peb.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/primitives/modules.h>
#include <sindri/primitives/status.h>
#include <sindri_hashes.h>
#include <stddef.h>

static snd_status_t WINAPI nt_load_library(const char *module_name, HMODULE *out_module) {
    SND_CHECK_NULL(out_module, module_name);

    *out_module = NULL;
    size_t  len = snd_strnlen(module_name, SND_MAX_PATH - 1);
    wchar_t wname[SND_MAX_PATH];
    snd_ascii_to_wide(wname, SND_MAX_PATH, module_name, len);

    PVOID        base   = NULL;
    snd_status_t status = snd_peb_get_module_base(wname, &base);
    if (SND_SUCCEEDED(status)) {
        *out_module = (HMODULE)base;
        return SND_OK;
    }

    FARPROC ldr_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_LDRLOADDLL, &ldr_addr));

    SND_LdrLoadDll_t   pfnLdrLoadDll = (SND_LdrLoadDll_t)ldr_addr;
    SND_UNICODE_STRING us;
    snd_init_unicode_string(&us, wname, len);

    HANDLE   hModule   = NULL;
    NTSTATUS nt_status = pfnLdrLoadDll(NULL, NULL, &us, &hModule);

    if (SND_NT_FAILURE(nt_status)) {
        return SND_ERR_NT_CTX(SND_STATUS_MODULE_LOAD_FAILED, nt_status, "Module Name: %s", module_name);
    }

    *out_module = (HMODULE)hModule;
    return SND_OK;
}

static snd_status_t WINAPI nt_get_proc_address(HMODULE hModule, const char *proc_name, FARPROC *out_proc) {
    SND_CHECK_NULL(out_proc, hModule, proc_name);

    *out_proc = NULL;

    snd_buffer_t buf = {.data = (PVOID)hModule, .size = SND_SYS_DLL_SIZE_DEFAULT};

    snd_pe_parser_t parser = {0};
    SND_TRY(snd_pe_parse(&buf, TRUE, &parser));

    return snd_pe_get_export_address(&parser, proc_name, out_proc, snd_peb_get_module_base);
}

static snd_status_t WINAPI nt_get_proc_address_hash(HMODULE hModule, DWORD proc_hash, FARPROC *out_proc) {
    SND_CHECK_NULL(out_proc, hModule, proc_hash);

    *out_proc = NULL;

    snd_buffer_t buf = {.data = (PVOID)hModule, .size = SND_SYS_DLL_SIZE_DEFAULT};

    snd_pe_parser_t parser = {0};
    SND_TRY(snd_pe_parse(&buf, TRUE, &parser));

    return snd_pe_get_export_address_hash(&parser, proc_hash, out_proc, snd_peb_get_module_base);
}

const snd_module_api_t snd_mod_nt = {.load_library     = nt_load_library,
                                     .get_proc_address = nt_get_proc_address,
                                     .get_module_base  = snd_peb_get_module_base,

                                     .get_proc_address_hash = nt_get_proc_address_hash,
                                     .get_module_base_hash  = snd_peb_get_module_base_hash};
