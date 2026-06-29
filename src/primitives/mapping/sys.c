#include <sindri/common/status.h>
#include <sindri/internal/nt/types.h>
#include <sindri/primitives/os_api.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>
#include <windows.h>

extern snd_syscall_invoker_t g_syscall_invoker;

static snd_status_t WINAPI sys_mapping_open(const wchar_t *section_name, HANDLE *out_handle) {
    if (!out_handle || !section_name)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTOPENSECTION, &entry);
    if (SND_FAILED(status))
        return status;

    SND_UNICODE_STRING us_name;
    snd_init_unicode_string(&us_name, section_name, wcslen(section_name));

    SND_OBJECT_ATTRIBUTES obj_attr;
    SND_InitializeObjectAttributes(&obj_attr, &us_name, SND_OBJ_CASE_INSENSITIVE, NULL, NULL);

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.arg1               = out_handle;
    args.arg2               = (PVOID)(ULONG_PTR)SECTION_MAP_READ;
    args.arg3               = &obj_attr;

    if (g_syscall_invoker == NULL) {
        return SND_ERR_CTX(SND_STATUS_NOT_INITIALIZED, "g_syscall_invoker is NULL");
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_SECTION_OPEN_FAILED, nt_status);
}

static snd_status_t WINAPI sys_mapping_view(HANDLE section_handle, PVOID *out_base, SIZE_T *out_size) {
    if (!section_handle || !out_base || !out_size)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTMAPVIEWOFSECTION, &entry);
    if (SND_FAILED(status))
        return status;

    PVOID         base_address   = NULL;
    SIZE_T        view_size      = 0;
    LARGE_INTEGER section_offset = {0};

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.arg1               = section_handle;
    args.arg2               = GetCurrentProcess();
    args.arg3               = &base_address;
    args.arg4               = (PVOID)0;
    args.arg5               = (PVOID)0;
    args.arg6               = &section_offset;
    args.arg7               = &view_size;
    args.arg8               = (PVOID)(ULONG_PTR)1;
    args.arg9               = (PVOID)0;
    args.arg10              = (PVOID)(ULONG_PTR)PAGE_READONLY;

    if (g_syscall_invoker == NULL) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    if (!SND_NT_SUCCESS(nt_status)) {
        return SND_ERR_NT(SND_STATUS_SECTION_MAP_FAILED, nt_status);
    }

    *out_base = base_address;
    *out_size = view_size;
    return SND_OK;
}

static snd_status_t WINAPI sys_mapping_close(HANDLE handle) {
    if (!handle)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    snd_syscall_entry_t entry  = {0};
    snd_status_t        status = snd_syscall_resolve(SND_HASH_NTCLOSE, &entry);
    if (SND_FAILED(status))
        return status;

    snd_syscall_args_t args = {0};
    args.ssn                = entry.wSystemCall;
    args.sys_addr           = entry.pSyscallAddr;
    args.arg1               = handle;

    if (g_syscall_invoker == NULL) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }
    NTSTATUS nt_status = g_syscall_invoker(&args);
    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_HANDLE_CLOSE_FAILED, nt_status);
}

const snd_mapping_api_t snd_map_sys = {.open = sys_mapping_open, .view = sys_mapping_view, .close = sys_mapping_close};
