#include <sindri/common/debug.h>
#include <sindri/internal/nt/base.h>
#include <sindri/internal/nt/process.h>
#include <sindri/internal/windows/context.h>
#include <sindri/internal/windows/types.h>
#include <sindri/primitives/os_api.h>
#include <sindri/primitives/status.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>

static snd_status_t WINAPI sys_queue_apc(HANDLE thread, PVOID apc_routine, PVOID apc_argument) {
    SND_CHECK_NULL(thread, apc_routine);

    snd_syscall_args_t args = {.arg1 = thread, .arg2 = apc_routine, .arg3 = apc_argument};

    NTSTATUS nt_status = 0;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTQUEUEAPCTHREAD, &args, &nt_status));

    SND_DEBUG_PRINT("[sys] NtQueueApcThread thread=%p routine=%p nt=0x%08lX\n", (PVOID)thread, apc_routine,
                    (unsigned long)nt_status);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_QUEUE_FAILED, nt_status);
}

static snd_status_t WINAPI sys_resume_thread(HANDLE thread) {
    SND_CHECK_NULL(thread);

    ULONG previous_suspend_count = 0;

    snd_syscall_args_t args = {.arg1 = thread, .arg2 = &previous_suspend_count};

    NTSTATUS nt_status = 0;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTRESUMETHREAD, &args, &nt_status));

    SND_DEBUG_PRINT("[sys] NtResumeThread thread=%p prev=%lu nt=0x%08lX\n", (PVOID)thread,
                    (unsigned long)previous_suspend_count, (unsigned long)nt_status);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_RESUME_FAILED, nt_status);
}

static snd_status_t WINAPI sys_suspend_thread(HANDLE thread) {
    SND_CHECK_NULL(thread);

    ULONG previous_suspend_count = 0;

    snd_syscall_args_t args = {.arg1 = thread, .arg2 = &previous_suspend_count};

    NTSTATUS nt_status = 0;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTSUSPENDTHREAD, &args, &nt_status));

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_SUSPEND_FAILED, nt_status);
}

static snd_status_t WINAPI sys_get_context(HANDLE thread, SND_THREAD_REGISTERS *out_regs) {
    SND_CHECK_NULL(thread, out_regs);

    SND_NATIVE_CONTEXT native = {0};
    native.ContextFlags       = SND_CONTEXT_ARCH_FLAGS;

    snd_syscall_args_t args = {.arg1 = thread, .arg2 = &native};

    NTSTATUS nt_status = 0;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTGETCONTEXTTHREAD, &args, &nt_status));
    SND_DEBUG_PRINT("[sys] NtGetContextThread thread=%p nt=0x%08lX\n", (PVOID)thread, (unsigned long)nt_status);
    if (!SND_NT_SUCCESS(nt_status)) {
        return SND_ERR_NT(SND_STATUS_THREAD_GET_CONTEXT_FAILED, nt_status);
    }

    snd_context_to_portable(&native, out_regs);
    return SND_OK;
}

static snd_status_t WINAPI sys_set_context(HANDLE thread, const SND_THREAD_REGISTERS *in_regs) {
    SND_CHECK_NULL(thread, in_regs);

    SND_NATIVE_CONTEXT native = {0};
    native.ContextFlags       = SND_CONTEXT_ARCH_FLAGS;

    snd_syscall_args_t get_args = {.arg1 = thread, .arg2 = &native};

    NTSTATUS nt_status = 0;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTGETCONTEXTTHREAD, &get_args, &nt_status));
    if (!SND_NT_SUCCESS(nt_status)) {
        return SND_ERR_NT(SND_STATUS_THREAD_GET_CONTEXT_FAILED, nt_status);
    }

    snd_context_from_portable(in_regs, &native);

    snd_syscall_args_t set_args = {.arg1 = thread, .arg2 = &native};
    nt_status                   = 0;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTSETCONTEXTTHREAD, &set_args, &nt_status));

    SND_DEBUG_PRINT("[sys] NtSetContextThread thread=%p ip=%p nt=0x%08lX\n", (PVOID)thread, (PVOID)in_regs->ip,
                    (unsigned long)nt_status);

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_THREAD_SET_CONTEXT_FAILED, nt_status);
}

static snd_status_t WINAPI sys_close_thread(HANDLE handle) {
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        return SND_OK;
    }

    snd_syscall_args_t args = {.arg1 = handle};

    NTSTATUS nt_status = 0;
    SND_TRY(snd_syscall_invoke(SND_HASH_NTCLOSE, &args, &nt_status));

    return SND_NT_SUCCESS(nt_status) ? SND_OK : SND_ERR_NT(SND_STATUS_HANDLE_CLOSE_FAILED, nt_status);
}

const snd_thread_api_t snd_thread_sys = {.queue_apc      = sys_queue_apc,
                                         .resume_thread  = sys_resume_thread,
                                         .suspend_thread = sys_suspend_thread,
                                         .get_context    = sys_get_context,
                                         .set_context    = sys_set_context,
                                         .close_handle   = sys_close_thread};
