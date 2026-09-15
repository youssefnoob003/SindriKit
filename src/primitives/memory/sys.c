#include <sindri/common/hash.h>
#include <sindri/common/macros.h>
#include <sindri/internal/nt/peb.h>
#include <sindri/internal/nt/process.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/primitives/memory.h>
#include <sindri/primitives/status.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>

void snd_buffer_free_sys(snd_buffer_t *buffer) {
    if (buffer == NULL || buffer->data == NULL) {
        return;
    }

    (void)snd_mem_sys.free(buffer->data, 0, SND_MEM_RELEASE);
}

static snd_status_t WINAPI sys_alloc(LPVOID address, SIZE_T size, DWORD allocation_type, DWORD protect,
                                     LPVOID *out_address) {
    SND_CHECK_NULL(out_address);
    *out_address = NULL;

    HANDLE processHandle = SND_CURRENT_PROCESS;
    PVOID  baseAddress   = address;
    SIZE_T regionSize    = size;

    snd_syscall_args_t args = {.arg1 = processHandle,
                               .arg2 = &baseAddress,
                               .arg3 = 0,
                               .arg4 = &regionSize,
                               .arg5 = (PVOID)(ULONG_PTR)allocation_type,
                               .arg6 = (PVOID)(ULONG_PTR)protect};

    NTSTATUS nt_status = 0;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTALLOCATEVIRTUALMEMORY, &args, &nt_status));

    if (SND_NT_FAILURE(nt_status)) {
        return SND_ERR_NT(SND_STATUS_ALLOC_FAILED, nt_status);
    }

    *out_address = baseAddress;
    return SND_OK;
}

static snd_status_t WINAPI sys_free(LPVOID address, SIZE_T size, DWORD free_type) {
    if (!address) {
        return SND_OK;
    }

    HANDLE processHandle = SND_CURRENT_PROCESS;
    PVOID  baseAddress   = address;
    SIZE_T regionSize    = (free_type == SND_MEM_RELEASE) ? 0 : size;

    snd_syscall_args_t args = {
        .arg1 = processHandle, .arg2 = &baseAddress, .arg3 = &regionSize, .arg4 = (PVOID)(ULONG_PTR)free_type};

    NTSTATUS nt_status = 0;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTFREEVIRTUALMEMORY, &args, &nt_status));

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_FREE_FAILED, nt_status);
}

static snd_status_t WINAPI sys_protect(LPVOID address, SIZE_T size, DWORD new_protect, DWORD *old_protect) {
    SND_CHECK_NULL(address, size);

    HANDLE processHandle = SND_CURRENT_PROCESS;
    PVOID  baseAddress   = address;
    SIZE_T regionSize    = size;

    DWORD  dummy_old_protect  = 0;
    PDWORD target_old_protect = old_protect ? old_protect : &dummy_old_protect;

    snd_syscall_args_t args = {.arg1 = processHandle,
                               .arg2 = &baseAddress,
                               .arg3 = &regionSize,
                               .arg4 = (PVOID)(ULONG_PTR)new_protect,
                               .arg5 = target_old_protect};

    NTSTATUS nt_status = 0;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTPROTECTVIRTUALMEMORY, &args, &nt_status));

    if (SND_NT_SUCCESS(nt_status)) {
        if (old_protect) {
            *old_protect = *target_old_protect;
        }
        return SND_OK;
    }
    return SND_ERR_NT(SND_STATUS_PROTECT_FAILED, nt_status);
}

const snd_memory_api_t snd_mem_sys = {.alloc = sys_alloc, .free = sys_free, .protect = sys_protect};
