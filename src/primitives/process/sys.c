#include <sindri/common/memory.h>
#include <sindri/common/string.h>
#include <sindri/internal/nt/base.h>
#include <sindri/internal/nt/peb.h>
#include <sindri/internal/nt/process.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/parsers/env/peb.h>
#include <sindri/primitives/process.h>
#include <sindri/primitives/status.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>

static snd_status_t WINAPI sys_create_process_params(const void *win32_path_unicode, const wchar_t *cmd_line,
                                                     PVOID *out_params) {
    const SND_UNICODE_STRING *win32_path = (const SND_UNICODE_STRING *)win32_path_unicode;
    SND_CHECK_NULL(win32_path, out_params);

    PSND_PEB peb = snd_peb_get_local();
    if (!peb) {
        return SND_ERR(SND_STATUS_PEB_LOCAL_NOT_FOUND);
    }

    PSND_RTL_USER_PROCESS_PARAMETERS parent_params = NULL;
    SND_TRY(snd_env_get_process_params(peb, &parent_params));

    PWSTR  env       = (PWSTR)parent_params->Environment;
    size_t env_chars = snd_wcsn_env_size(env, SND_UNICODE_STRING_MAX_CHARS);
    size_t env_bytes = env_chars * sizeof(wchar_t);

    SND_UNICODE_STRING u_cmd = {0};
    if (cmd_line) {
        size_t len_chars    = snd_wcsnlen(cmd_line, SND_UNICODE_STRING_MAX_CHARS);
        u_cmd.Length        = (USHORT)(len_chars * sizeof(wchar_t));
        u_cmd.MaximumLength = u_cmd.Length + sizeof(wchar_t);
        u_cmd.Buffer        = (PWSTR)cmd_line;
    } else {
        u_cmd = *win32_path;
    }

    SIZE_T header_size = sizeof(SND_RTL_USER_PROCESS_PARAMETERS_FULL);
    SIZE_T total_size  = header_size + win32_path->MaximumLength + u_cmd.MaximumLength +
                         parent_params->CurrentDirectory.DosPath.MaximumLength + env_bytes;

    PVOID  params_base = NULL;
    SIZE_T alloc_size  = total_size;

    snd_syscall_args_t alloc_args = {.arg1 = (HANDLE)(LONG_PTR)SND_CURRENT_PROCESS,
                                     .arg2 = &params_base,
                                     .arg3 = 0,
                                     .arg4 = &alloc_size,
                                     .arg5 = (PVOID)(ULONG_PTR)(SND_MEM_COMMIT | SND_MEM_RESERVE),
                                     .arg6 = (PVOID)(ULONG_PTR)SND_PAGE_READWRITE};

    NTSTATUS nt_status;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTALLOCATEVIRTUALMEMORY, &alloc_args, &nt_status));
    if (SND_NT_FAILURE(nt_status)) {
        return SND_ERR_NT(SND_STATUS_PROCESS_CREATE_PARAMS_FAILED, nt_status);
    }

    snd_memzero(params_base, alloc_size);

    PSND_RTL_USER_PROCESS_PARAMETERS_FULL params = (PSND_RTL_USER_PROCESS_PARAMETERS_FULL)params_base;
    params->MaximumLength                        = (ULONG)total_size;
    params->Length                               = (ULONG)total_size;
    params->Flags                                = 1; // RTL_USER_PROC_PARAMS_NORMALIZED

    params->ConsoleHandle  = parent_params->ConsoleHandle;
    params->ConsoleFlags   = parent_params->ConsoleFlags;
    params->StandardInput  = parent_params->StandardInput;
    params->StandardOutput = parent_params->StandardOutput;
    params->StandardError  = parent_params->StandardError;

    PBYTE cursor = (PBYTE)params_base + header_size;

    // 1. ImagePathName
    params->ImagePathName.Length        = win32_path->Length;
    params->ImagePathName.MaximumLength = win32_path->MaximumLength;
    params->ImagePathName.Buffer        = (PWSTR)cursor;
    if (win32_path->Buffer && win32_path->Length > 0) {
        snd_memcpy(cursor, win32_path->Buffer, win32_path->Length);
    }
    cursor += win32_path->MaximumLength;

    // 2. CommandLine
    params->CommandLine.Length        = u_cmd.Length;
    params->CommandLine.MaximumLength = u_cmd.MaximumLength;
    params->CommandLine.Buffer        = (PWSTR)cursor;
    if (u_cmd.Buffer && u_cmd.Length > 0) {
        snd_memcpy(cursor, u_cmd.Buffer, u_cmd.Length);
    }
    cursor += u_cmd.MaximumLength;

    // 3. CurrentDirectory
    params->CurrentDirectory.DosPath.Length        = parent_params->CurrentDirectory.DosPath.Length;
    params->CurrentDirectory.DosPath.MaximumLength = parent_params->CurrentDirectory.DosPath.MaximumLength;
    params->CurrentDirectory.DosPath.Buffer        = (PWSTR)cursor;
    if (parent_params->CurrentDirectory.DosPath.Buffer && parent_params->CurrentDirectory.DosPath.Length > 0) {
        snd_memcpy(cursor, parent_params->CurrentDirectory.DosPath.Buffer,
                   parent_params->CurrentDirectory.DosPath.Length);
    }
    cursor += parent_params->CurrentDirectory.DosPath.MaximumLength;

    // 4. Environment
    if (env_bytes > 0 && env) {
        params->Environment = cursor;
        snd_memcpy(cursor, env, env_bytes);
        *(ULONG_PTR *)((PBYTE)params + SND_ENVSIZE_OFFSET) = env_bytes;
    }

    *out_params = params_base;
    return SND_OK;
}

static snd_status_t WINAPI sys_create_process(const snd_process_api_t *api, const wchar_t *image_path,
                                              const wchar_t *command_line, HANDLE *out_process, HANDLE *out_thread) {
    SND_CHECK_NULL(out_process, out_thread, image_path);
    *out_process = NULL;
    *out_thread  = NULL;

    // 1. Win32 path for RTL_USER_PROCESS_PARAMETERS
    size_t             img_len_chars = snd_wcsnlen(image_path, SND_UNICODE_STRING_MAX_CHARS);
    SND_UNICODE_STRING u_win32_path  = {0};
    u_win32_path.Length              = (USHORT)(img_len_chars * sizeof(wchar_t));
    u_win32_path.MaximumLength       = u_win32_path.Length + sizeof(wchar_t);
    u_win32_path.Buffer              = (PWSTR)image_path;

    // 2. NT path (\??\C:\...) for Attribute List
    wchar_t nt_path[SND_MAX_PATH] = L"\\??\\";
    snd_wcsncpy(nt_path + 4, SND_MAX_PATH - 4, image_path, SND_MAX_PATH - 4);

    size_t             nt_len_chars = snd_wcsnlen(nt_path, SND_UNICODE_STRING_MAX_CHARS);
    SND_UNICODE_STRING u_nt_path    = {0};
    u_nt_path.Length                = (USHORT)(nt_len_chars * sizeof(wchar_t));
    u_nt_path.MaximumLength         = u_nt_path.Length + sizeof(wchar_t);
    u_nt_path.Buffer                = nt_path;

    PVOID process_params = NULL;
    if (api && api->create_process_params) {
        SND_TRY(api->create_process_params(&u_win32_path, command_line, &process_params));
    }

    // 3. SND_PS_CREATE_INFO
    SND_PS_CREATE_INFO create_info;
    snd_memzero(&create_info, sizeof(create_info));
    create_info.Size                                          = sizeof(SND_PS_CREATE_INFO);
    create_info.State                                         = SND_PS_CREATE_INITIAL_STATE;
    create_info.StateUnion.InitState.InitFlagsUnion.InitFlags = 3;

    // 4. SND_PS_ATTRIBUTE_LIST setup
    SND_PS_ATTRIBUTE_LIST attr_list;
    snd_memzero(&attr_list, sizeof(attr_list));
    attr_list.TotalLength                    = sizeof(SND_PS_ATTRIBUTE_LIST);
    attr_list.Attributes[0].Attribute        = SND_PS_ATTRIBUTE_IMAGE_NAME;
    attr_list.Attributes[0].Size             = u_nt_path.Length;            // e.g. 70
    attr_list.Attributes[0].ValueUnion.Value = (ULONG_PTR)u_nt_path.Buffer; // L"\\??\\C:\\..."
    attr_list.Attributes[0].ReturnLength     = NULL;

    snd_syscall_args_t args = {.arg1  = out_process,
                               .arg2  = out_thread,
                               .arg3  = (PVOID)(ULONG_PTR)SND_PROCESS_ALL_ACCESS,
                               .arg4  = (PVOID)(ULONG_PTR)SND_THREAD_ALL_ACCESS,
                               .arg5  = NULL,
                               .arg6  = NULL,
                               .arg7  = (PVOID)(ULONG_PTR)0,
                               .arg8  = (PVOID)(ULONG_PTR)1,
                               .arg9  = process_params,
                               .arg10 = &create_info,
                               .arg11 = &attr_list};

    NTSTATUS nt_status;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTCREATEUSERPROCESS, &args, &nt_status));

    if (api && api->free_process_params && process_params) {
        api->free_process_params(process_params);
    }

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_CREATE_FAILED, nt_status);
}

static snd_status_t WINAPI sys_free_process_params(PVOID params) {
    if (!params) {
        return SND_OK;
    }

    PVOID  base        = params;
    SIZE_T region_size = 0;

    snd_syscall_args_t free_args = {.arg1 = (HANDLE)(LONG_PTR)SND_CURRENT_PROCESS,
                                    .arg2 = &base,
                                    .arg3 = &region_size,
                                    .arg4 = (PVOID)(ULONG_PTR)SND_MEM_RELEASE};

    NTSTATUS nt_status;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTFREEVIRTUALMEMORY, &free_args, &nt_status));

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_FREE_PARAMS_FAILED, nt_status);
}

static snd_status_t WINAPI sys_open_process(DWORD pid, DWORD desired_access, HANDLE *out_process) {
    SND_CHECK_NULL(out_process);
    *out_process = NULL;

    SND_CLIENT_ID         cid = {.UniqueProcess = (HANDLE)(ULONG_PTR)pid};
    SND_OBJECT_ATTRIBUTES oa  = {0};
    SND_InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);

    snd_syscall_args_t args = {
        .arg1 = out_process, .arg2 = (PVOID)(ULONG_PTR)desired_access, .arg3 = &oa, .arg4 = &cid};

    NTSTATUS nt_status;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTOPENPROCESS, &args, &nt_status));

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_OPEN_FAILED, nt_status);
}

static snd_status_t WINAPI sys_alloc_remote(HANDLE process, SIZE_T size, DWORD allocation_type, DWORD protect,
                                            PVOID *out_address) {
    SND_CHECK_NULL(process, out_address);
    *out_address = NULL;

    PVOID  local_base = NULL;
    SIZE_T local_size = size;

    snd_syscall_args_t args = {.arg1 = process,
                               .arg2 = &local_base,
                               .arg3 = 0,
                               .arg4 = &local_size,
                               .arg5 = (PVOID)(ULONG_PTR)allocation_type,
                               .arg6 = (PVOID)(ULONG_PTR)protect};

    NTSTATUS nt_status;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTALLOCATEVIRTUALMEMORY, &args, &nt_status));

    if (SND_NT_FAILURE(nt_status)) {
        return SND_ERR_NT(SND_STATUS_PROCESS_REMOTE_ALLOC_FAILED, nt_status);
    }

    *out_address = local_base;
    return SND_OK;
}

static snd_status_t WINAPI sys_write_remote(HANDLE process, PVOID base_address, const void *buffer, SIZE_T size,
                                            SIZE_T *bytes_written) {
    SND_CHECK_NULL(process, base_address, buffer);

    SIZE_T written = 0;

    snd_syscall_args_t args = {
        .arg1 = process, .arg2 = base_address, .arg3 = (PVOID)buffer, .arg4 = (PVOID)size, .arg5 = &written};

    NTSTATUS nt_status;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTWRITEVIRTUALMEMORY, &args, &nt_status));

    if (bytes_written) {
        *bytes_written = written;
    }

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_REMOTE_WRITE_FAILED, nt_status);
}

static snd_status_t WINAPI sys_protect_remote(HANDLE process, PVOID base_address, SIZE_T size, DWORD new_protect,
                                              DWORD *old_protect) {
    SND_CHECK_NULL(process, base_address);

    PVOID  addr       = base_address;
    SIZE_T regionSize = size;
    ULONG  oldProt    = 0;

    snd_syscall_args_t args = {
        .arg1 = process, .arg2 = &addr, .arg3 = &regionSize, .arg4 = (PVOID)(ULONG_PTR)new_protect, .arg5 = &oldProt};

    NTSTATUS nt_status;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTPROTECTVIRTUALMEMORY, &args, &nt_status));

    if (old_protect) {
        *old_protect = oldProt;
    }

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_PROCESS_REMOTE_PROTECT_FAILED, nt_status);
}

static snd_status_t WINAPI sys_create_remote_thread(HANDLE process, PVOID start_address, PVOID parameter,
                                                    HANDLE *out_thread) {
    SND_CHECK_NULL(out_thread, process, start_address);
    *out_thread = NULL;

    snd_syscall_args_t args = {.arg1 = out_thread,
                               .arg2 = (PVOID)(ULONG_PTR)SND_THREAD_ALL_ACCESS,
                               .arg3 = NULL,
                               .arg4 = process,
                               .arg5 = start_address,
                               .arg6 = parameter};

    NTSTATUS nt_status;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTCREATETHREADEX, &args, &nt_status));

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_REMOTE_CREATE_FAILED, nt_status);
}

static snd_status_t WINAPI sys_close_handle(HANDLE handle) {
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        return SND_OK;
    }

    snd_syscall_args_t args = {.arg1 = handle};

    NTSTATUS nt_status;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTCLOSE, &args, &nt_status));

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_HANDLE_CLOSE_FAILED, nt_status);
}

const snd_process_api_t snd_proc_sys = {.create_process_params = sys_create_process_params,
                                        .free_process_params   = sys_free_process_params,
                                        .create_process        = sys_create_process,
                                        .open_process          = sys_open_process,
                                        .alloc_remote          = sys_alloc_remote,
                                        .write_remote          = sys_write_remote,
                                        .protect_remote        = sys_protect_remote,
                                        .create_remote_thread  = sys_create_remote_thread,
                                        .close_handle          = sys_close_handle};
