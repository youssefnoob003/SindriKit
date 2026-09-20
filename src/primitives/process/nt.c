#include <sindri/common/macros.h>
#include <sindri/common/string.h>
#include <sindri/internal/nt/api.h>
#include <sindri/internal/nt/base.h>
#include <sindri/internal/nt/process.h>
#include <sindri/internal/windows/types.h>
#include <sindri/parsers/env/ntdll.h>
#include <sindri/primitives/process.h>
#include <sindri/primitives/status.h>
#include <sindri_hashes.h>

static snd_status_t WINAPI nt_create_process_params(const void *nt_path_unicode, const wchar_t *cmd_line,
                                                    PVOID *out_params) {
    const SND_UNICODE_STRING *nt_path    = (const SND_UNICODE_STRING *)nt_path_unicode;
    FARPROC                   pfn_create = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_RTLCREATEPROCESSPARAMETERSEX, &pfn_create));

    SND_UNICODE_STRING u_cmd = {0};
    if (cmd_line) {
        snd_init_unicode_string(&u_cmd, cmd_line, snd_wcsnlen(cmd_line, SND_UNICODE_STRING_MAX_CHARS));
    }

    SND_RtlCreateProcessParametersEx_t pRtlCreate = (SND_RtlCreateProcessParametersEx_t)pfn_create;
    NTSTATUS nt_status = pRtlCreate(out_params, (PSND_UNICODE_STRING)nt_path, NULL, NULL,
                                    cmd_line ? &u_cmd : (PSND_UNICODE_STRING)nt_path, NULL, NULL, NULL, NULL, NULL, 1);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_CREATE_PARAMS_FAILED, nt_status);
}

static snd_status_t WINAPI nt_free_process_params(PVOID params) {
    if (!params) {
        return SND_OK;
    }

    FARPROC pfn_destroy = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_RTLDESTROYPROCESSPARAMETERS, &pfn_destroy));

    SND_RtlDestroyProcessParameters_t pRtlDestroy = (SND_RtlDestroyProcessParameters_t)pfn_destroy;
    NTSTATUS                          nt_status   = pRtlDestroy(params);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_FREE_PARAMS_FAILED, nt_status);
}

static snd_status_t WINAPI nt_create_process(const snd_process_api_t *api, const wchar_t *image_path,
                                             const wchar_t *command_line, HANDLE *out_process, HANDLE *out_thread) {
    SND_CHECK_NULL(out_process, out_thread, image_path);
    *out_process = NULL;
    *out_thread  = NULL;

    FARPROC pfn_create_user_proc = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTCREATEUSERPROCESS, &pfn_create_user_proc));

    SND_UNICODE_STRING u_win32_path = {0};
    snd_init_unicode_string(&u_win32_path, image_path, snd_wcsnlen(image_path, SND_UNICODE_STRING_MAX_CHARS));

    wchar_t nt_path[SND_MAX_PATH] = L"\\??\\";
    snd_wcsncpy(nt_path + 4, SND_MAX_PATH - 4, image_path, SND_MAX_PATH - 4);

    SND_UNICODE_STRING u_nt_path = {0};
    snd_init_unicode_string(&u_nt_path, nt_path, snd_wcsnlen(nt_path, SND_UNICODE_STRING_MAX_CHARS));

    PVOID process_params = NULL;
    if (api && api->create_process_params) {
        SND_TRY(api->create_process_params(&u_win32_path, command_line, &process_params));
    }

    SND_PS_CREATE_INFO create_info = {.Size = sizeof(create_info), .State = SND_PS_CREATE_INITIAL_STATE};

    SND_PS_ATTRIBUTE_LIST attr_list             = {0};
    attr_list.TotalLength                       = sizeof(SND_PS_ATTRIBUTE_LIST);
    attr_list.Attributes[0].Attribute           = SND_PS_ATTRIBUTE_IMAGE_NAME;
    attr_list.Attributes[0].Size                = u_nt_path.Length;
    attr_list.Attributes[0].ValueUnion.ValuePtr = u_nt_path.Buffer;
    attr_list.Attributes[0].ReturnLength        = NULL;

    SND_NtCreateUserProcess_t pNtCreateUserProcess = (SND_NtCreateUserProcess_t)pfn_create_user_proc;
    NTSTATUS                  nt_status =
        pNtCreateUserProcess(out_process, out_thread, SND_PROCESS_ALL_ACCESS, SND_THREAD_ALL_ACCESS, NULL, NULL,
                             0, // ProcessFlags = 0
                             SND_THREAD_CREATE_FLAGS_CREATE_SUSPENDED, process_params, &create_info, &attr_list);

    if (api && api->free_process_params) {
        api->free_process_params(process_params);
    }

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_CREATE_FAILED, nt_status);
}

static snd_status_t WINAPI nt_open_process(DWORD pid, DWORD desired_access, HANDLE *out_process) {
    SND_CHECK_NULL(out_process);
    *out_process = NULL;

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTOPENPROCESS, &func_addr));

    SND_CLIENT_ID         cid = {.UniqueProcess = (HANDLE)(ULONG_PTR)pid, .UniqueThread = 0};
    SND_OBJECT_ATTRIBUTES oa  = {0};
    SND_InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);

    SND_NtOpenProcess_t pNtOpenProcess = (SND_NtOpenProcess_t)func_addr;
    NTSTATUS            nt_status      = pNtOpenProcess(out_process, desired_access, &oa, (PVOID)&cid);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_OPEN_FAILED, nt_status);
}

static snd_status_t WINAPI nt_alloc_remote(HANDLE process, SIZE_T size, DWORD allocation_type, DWORD protect,
                                           PVOID *out_address) {
    SND_CHECK_NULL(process, out_address);
    *out_address = NULL;

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTALLOCATEVIRTUALMEMORY, &func_addr));

    PVOID  local_base = NULL;
    SIZE_T local_size = size;

    SND_NtAllocateVirtualMemory_t pNtAllocateVirtualMemory = (SND_NtAllocateVirtualMemory_t)func_addr;
    NTSTATUS nt_status = pNtAllocateVirtualMemory(process, &local_base, 0, &local_size, allocation_type, protect);

    if (SND_NT_FAILURE(nt_status)) {
        return SND_ERR_NT(SND_STATUS_PROCESS_REMOTE_ALLOC_FAILED, nt_status);
    }

    *out_address = local_base;
    return SND_OK;
}

static snd_status_t WINAPI nt_free_remote(HANDLE process, PVOID base_address, SIZE_T size, DWORD free_type) {
    SND_CHECK_NULL(process, base_address);

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTFREEVIRTUALMEMORY, &func_addr));

    PVOID                     address              = base_address;
    SIZE_T                    region_size          = size;
    SND_NtFreeVirtualMemory_t pNtFreeVirtualMemory = (SND_NtFreeVirtualMemory_t)func_addr;
    NTSTATUS                  nt_status            = pNtFreeVirtualMemory(process, &address, &region_size, free_type);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_REMOTE_FREE_FAILED, nt_status);
}

static snd_status_t WINAPI nt_write_remote(HANDLE process, PVOID base_address, const void *buffer, SIZE_T size,
                                           SIZE_T *bytes_written) {
    SND_CHECK_NULL(process, base_address, buffer);

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTWRITEVIRTUALMEMORY, &func_addr));

    SIZE_T                     written               = 0;
    SND_NtWriteVirtualMemory_t pNtWriteVirtualMemory = (SND_NtWriteVirtualMemory_t)func_addr;
    NTSTATUS                   nt_status = pNtWriteVirtualMemory(process, base_address, (PVOID)buffer, size, &written);

    if (bytes_written) {
        *bytes_written = written;
    }

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_REMOTE_WRITE_FAILED, nt_status);
}

static snd_status_t WINAPI nt_protect_remote(HANDLE process, PVOID base_address, SIZE_T size, DWORD new_protect,
                                             DWORD *old_protect) {
    SND_CHECK_NULL(process, base_address);

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTPROTECTVIRTUALMEMORY, &func_addr));

    PVOID  addr       = base_address;
    SIZE_T regionSize = size;
    ULONG  oldProt    = 0;

    SND_NtProtectVirtualMemory_t pNtProtectVirtualMemory = (SND_NtProtectVirtualMemory_t)func_addr;
    NTSTATUS nt_status = pNtProtectVirtualMemory(process, &addr, &regionSize, new_protect, &oldProt);

    if (old_protect) {
        *old_protect = oldProt;
    }

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_REMOTE_PROTECT_FAILED, nt_status);
}

static snd_status_t WINAPI nt_create_remote_thread(HANDLE process, PVOID start_address, PVOID parameter,
                                                   HANDLE *out_thread) {
    SND_CHECK_NULL(out_thread, process, start_address);
    *out_thread = NULL;

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTCREATETHREADEX, &func_addr));

    SND_NtCreateThreadEx_t pNtCreateThreadEx = (SND_NtCreateThreadEx_t)func_addr;
    NTSTATUS               nt_status =
        pNtCreateThreadEx(out_thread, SND_THREAD_ALL_ACCESS, NULL, process, start_address, parameter, 0, 0, 0, 0, NULL);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_REMOTE_CREATE_FAILED, nt_status);
}

static snd_status_t WINAPI nt_terminate_process(HANDLE process, UINT exit_code) {
    SND_CHECK_NULL(process);

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTTERMINATEPROCESS, &func_addr));

    SND_NtTerminateProcess_t pNtTerminateProcess = (SND_NtTerminateProcess_t)func_addr;
    NTSTATUS                 nt_status           = pNtTerminateProcess(process, (NTSTATUS)exit_code);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_TERMINATE_FAILED, nt_status);
}

static snd_status_t WINAPI nt_close_handle(HANDLE handle) {
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        return SND_OK;
    }

    FARPROC func_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(SND_HASH_NTCLOSE, &func_addr));

    SND_NtClose_t pNtClose  = (SND_NtClose_t)func_addr;
    NTSTATUS      nt_status = pNtClose(handle);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_HANDLE_CLOSE_FAILED, nt_status);
}

const snd_process_api_t snd_proc_nt = {.create_process_params = nt_create_process_params,
                                       .free_process_params   = nt_free_process_params,
                                       .create_process        = nt_create_process,
                                       .open_process          = nt_open_process,
                                       .alloc_remote          = nt_alloc_remote,
                                       .free_remote           = nt_free_remote,
                                       .write_remote          = nt_write_remote,
                                       .protect_remote        = nt_protect_remote,
                                       .create_remote_thread  = nt_create_remote_thread,
                                       .terminate_process     = nt_terminate_process,
                                       .close_handle          = nt_close_handle};
