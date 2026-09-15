#include <sindri/internal/nt/api.h>
#include <sindri/internal/nt/base.h>
#include <sindri/internal/nt/process.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/parsers/env/ntdll.h>
#include <sindri/primitives/memory.h>
#include <sindri/primitives/os_api.h>
#include <sindri/primitives/status.h>
#include <sindri_hashes.h>

void snd_buffer_free_nt(snd_buffer_t *buffer) {
    if (buffer == NULL || buffer->data == NULL) {
        return;
    }

    (void)snd_mem_nt.free(buffer->data, 0, SND_MEM_RELEASE);
}

static snd_status_t WINAPI nt_alloc(LPVOID address, SIZE_T size, DWORD allocation_type, DWORD protect,
                                    LPVOID *out_address) {
    SND_CHECK_NULL(out_address);

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTALLOCATEVIRTUALMEMORY, &func_addr));

    SND_NtAllocateVirtualMemory_t pNtVirtualAlloc = (SND_NtAllocateVirtualMemory_t)func_addr;
    NTSTATUS nt_status = pNtVirtualAlloc(SND_CURRENT_PROCESS, &address, 0, &size, allocation_type, protect);

    if (SND_NT_FAILURE(nt_status)) {
        return SND_ERR_NT(SND_STATUS_ALLOC_FAILED, nt_status);
    }

    *out_address = address;
    return SND_OK;
}

static snd_status_t WINAPI nt_free(LPVOID address, SIZE_T size, DWORD free_type) {
    if (!address) {
        return SND_OK;
    }

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTFREEVIRTUALMEMORY, &func_addr));

    SND_NtFreeVirtualMemory_t pNtFreeVirtualMemory = (SND_NtFreeVirtualMemory_t)func_addr;
    NTSTATUS                  nt_status = pNtFreeVirtualMemory(SND_CURRENT_PROCESS, &address, &size, free_type);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_FREE_FAILED, nt_status);
}

static snd_status_t WINAPI nt_protect(LPVOID address, SIZE_T size, DWORD new_protect, DWORD *old_protect) {
    if (!address || size == 0) {
        return SND_OK;
    }

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTPROTECTVIRTUALMEMORY, &func_addr));

    DWORD  dummy_old_protect  = 0;
    PDWORD target_old_protect = old_protect ? old_protect : &dummy_old_protect;

    SND_NtProtectVirtualMemory_t pNtProtectVirtualMemory = (SND_NtProtectVirtualMemory_t)func_addr;
    NTSTATUS nt_status = pNtProtectVirtualMemory(SND_CURRENT_PROCESS, &address, &size, new_protect, target_old_protect);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROTECT_FAILED, nt_status);
}

const snd_memory_api_t snd_mem_nt = {.alloc = nt_alloc, .free = nt_free, .protect = nt_protect};
