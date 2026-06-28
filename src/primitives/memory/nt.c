#include <sindri/common/status.h>
#include <sindri/internal/nt/api.h>
#include <sindri/internal/nt/types.h>
#include <sindri/parsers/env/peb.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/primitives/memory.h>
#include <sindri/primitives/os_api.h>
#include <sindri_hashes.h>
#include <windows.h>

static snd_status_t WINAPI nt_alloc(LPVOID address, SIZE_T size, DWORD allocation_type, DWORD protect,
                                    LPVOID *out_address) {
    if (!out_address)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status = snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTALLOCATEVIRTUALMEMORY,
                                            &func_addr, NULL);
    if (SND_FAILED(status))
        return status;

    SND_NtAllocateVirtualMemory_t pNtVirtualAlloc = (SND_NtAllocateVirtualMemory_t)func_addr;

    NTSTATUS nt_status = pNtVirtualAlloc(GetCurrentProcess(), &address, 0, &size, allocation_type, protect);

    if (SND_NT_SUCCESS(nt_status)) {
        *out_address = address;
        return SND_OK;
    }
    return SND_ERR_NT(SND_STATUS_ALLOC_FAILED, nt_status);
}

static snd_status_t WINAPI nt_free(LPVOID address, SIZE_T size, DWORD free_type) {
    if (!address || !size)
        return SND_OK;

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status =
        snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTFREEVIRTUALMEMORY, &func_addr, NULL);
    if (SND_FAILED(status))
        return status;

    SND_NtFreeVirtualMemory_t pNtFreeVirtualMemory = (SND_NtFreeVirtualMemory_t)func_addr;

    NTSTATUS nt_status = pNtFreeVirtualMemory(GetCurrentProcess(), &address, &size, free_type);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_VIRTUAL_FREE_FAILED, nt_status);
}

static snd_status_t WINAPI nt_protect(LPVOID address, SIZE_T size, DWORD new_protect, DWORD *old_protect) {
    if (!address || !size)
        return SND_OK;

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status            = snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTPROTECTVIRTUALMEMORY,
                                                       &func_addr, NULL);
    if (SND_FAILED(status))
        return status;

    SND_NtProtectVirtualMemory_t pNtProtectVirtualMemory = (SND_NtProtectVirtualMemory_t)func_addr;

    NTSTATUS nt_status = pNtProtectVirtualMemory(GetCurrentProcess(), &address, &size, new_protect, old_protect);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROTECTION_UPDATE_FAILED, nt_status);
}

const snd_memory_api_t snd_mem_nt = {.alloc = nt_alloc, .free = nt_free, .protect = nt_protect};
