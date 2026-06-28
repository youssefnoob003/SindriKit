#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/internal/nt/api.h>
#include <sindri/parsers/env/peb.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/primitives/process.h>
#include <sindri_hashes.h>
#include <windows.h>
#include <winerror.h>

static snd_status_t WINAPI nt_open_process(DWORD pid, DWORD desired_access, HANDLE *out_process) {
    if (!out_process)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status = snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTOPENPROCESS, &func_addr, NULL);
    if (SND_FAILED(status))
        return status;

    SND_CLIENT_ID cid = {0};
    cid.UniqueProcess = (HANDLE)(ULONG_PTR)pid;
    cid.UniqueThread  = 0;

    SND_OBJECT_ATTRIBUTES oa = {0};
    SND_InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);

    SND_NtOpenProcess_t pNtOpenProcess = (SND_NtOpenProcess_t)func_addr;
    NTSTATUS            nt_status      = pNtOpenProcess(out_process, desired_access, &oa, (PVOID)&cid);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_OPEN_FAILED, nt_status);
}

static snd_status_t WINAPI nt_alloc_remote(HANDLE process, SIZE_T size, DWORD allocation_type, DWORD protect,
                                           PVOID *out_address) {
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

    PVOID  local_base = NULL;
    SIZE_T local_size = size;

    SND_NtAllocateVirtualMemory_t pNtAllocateVirtualMemory = (SND_NtAllocateVirtualMemory_t)func_addr;
    NTSTATUS nt_status = pNtAllocateVirtualMemory(process, &local_base, 0, &local_size, allocation_type, protect);
    if (SND_NT_SUCCESS(nt_status)) {
        *out_address = local_base;
        return SND_OK;
    }
    return SND_ERR_NT(SND_STATUS_PROCESS_OPEN_FAILED, nt_status);
}

static snd_status_t WINAPI nt_write_remote(HANDLE process, PVOID base_address, const void *buffer, SIZE_T size,
                                           SIZE_T *bytes_written) {
    SIZE_T written = 0;

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status = snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTWRITEVIRTUALMEMORY, &func_addr,
                                            NULL);
    if (SND_FAILED(status))
        return status;

    SND_NtWriteVirtualMemory_t pNtWriteVirtualMemory = (SND_NtWriteVirtualMemory_t)func_addr;
    NTSTATUS                   nt_status = pNtWriteVirtualMemory(process, base_address, (PVOID)buffer, size, &written);

    if (bytes_written)
        *bytes_written = written;

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_VIRTUAL_WRITE_FAILED, nt_status);
}

static snd_status_t WINAPI nt_protect_remote(HANDLE process, PVOID base_address, SIZE_T size, DWORD new_protect,
                                             DWORD *old_protect) {
    PVOID  addr       = base_address;
    SIZE_T regionSize = size;
    ULONG  oldProt    = 0;

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status            = snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTPROTECTVIRTUALMEMORY,
                                                       &func_addr, NULL);
    if (SND_FAILED(status))
        return status;

    SND_NtProtectVirtualMemory_t pNtProtectVirtualMemory_t = (SND_NtProtectVirtualMemory_t)func_addr;
    NTSTATUS nt_status = pNtProtectVirtualMemory_t(process, &addr, &regionSize, new_protect, &oldProt);

    if (old_protect)
        *old_protect = oldProt;

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_VIRTUAL_PROTECT_FAILED, nt_status);
}

static snd_status_t WINAPI nt_create_remote_thread(HANDLE process, PVOID start_address, PVOID parameter,
                                                   HANDLE *out_thread) {
    if (!out_thread)
        return SND_ERR(SND_STATUS_NULL_POINTER);
    *out_thread = NULL;

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status =
        snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTCREATETHREADEX, &func_addr, NULL);
    if (SND_FAILED(status))
        return status;

    SND_NtCreateThreadEx_t pNtCreateThreadEx = (SND_NtCreateThreadEx_t)func_addr;
    NTSTATUS               nt_status =
        pNtCreateThreadEx(out_thread, 0x1FFFFF, NULL, process, start_address, parameter, 0, 0, 0, 0, NULL);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_CREATE_FAILED, nt_status);
}

static snd_status_t WINAPI nt_close_handle(HANDLE handle) {
    if (!handle)
        return SND_OK;

    PVOID        ntdll;
    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status))
        return status;

    FARPROC func_addr = NULL;
    status = snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTCLOSE, &func_addr, NULL);
    if (SND_FAILED(status))
        return status;

    SND_NtClose_t pNtClose  = (SND_NtClose_t)func_addr;
    NTSTATUS      nt_status = pNtClose(handle);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_HANDLE_CLOSE_FAILED, nt_status);
}

const snd_process_api_t snd_proc_nt = {.open_process         = nt_open_process,
                                       .alloc_remote         = nt_alloc_remote,
                                       .write_remote         = nt_write_remote,
                                       .protect_remote       = nt_protect_remote,
                                       .create_remote_thread = nt_create_remote_thread,
                                       .close_handle         = nt_close_handle};
