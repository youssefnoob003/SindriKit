#include <sindri/internal/windows/context.h>
#include <sindri/primitives/os_api.h>
#include <sindri/primitives/status.h>
#include <windows.h>

static snd_status_t WINAPI win_queue_apc(HANDLE thread, PVOID apc_routine, PVOID apc_argument) {
    SND_CHECK_NULL(thread, apc_routine);

    DWORD result = QueueUserAPC((PAPCFUNC)apc_routine, thread, (ULONG_PTR)apc_argument);
    return result != 0 ? SND_OK : SND_ERR_W32(SND_STATUS_THREAD_QUEUE_FAILED);
}

static snd_status_t WINAPI win_resume_thread(HANDLE thread) {
    SND_CHECK_NULL(thread);

    DWORD suspend_count = ResumeThread(thread);
    return suspend_count != (DWORD)-1 ? SND_OK : SND_ERR_W32(SND_STATUS_THREAD_RESUME_FAILED);
}

static snd_status_t WINAPI win_suspend_thread(HANDLE thread) {
    SND_CHECK_NULL(thread);

    DWORD suspend_count = SuspendThread(thread);
    return suspend_count != (DWORD)-1 ? SND_OK : SND_ERR_W32(SND_STATUS_THREAD_SUSPEND_FAILED);
}

#if defined(SND_USE_WINDOWS_SDK)

#define SND_CONTEXT_PARITY_ASSERT(cond, name) typedef char name[(cond) ? 1 : -1]
#define SND_CONTEXT_PARITY(field)                                                                                      \
    SND_CONTEXT_PARITY_ASSERT((size_t)offsetof(SND_NATIVE_CONTEXT, field) == (size_t)FIELD_OFFSET(CONTEXT, field),     \
                              snd_ctx_parity_##field)

SND_CONTEXT_PARITY(ContextFlags);
SND_CONTEXT_PARITY(SND_CTX_FLD_IP);
SND_CONTEXT_PARITY(SND_CTX_FLD_SP);
SND_CONTEXT_PARITY(SND_CTX_FLD_CX);
SND_CONTEXT_PARITY(SND_CTX_FLD_DX);
SND_CONTEXT_PARITY(SND_CTX_FLD_FLAGS);
SND_CONTEXT_PARITY_ASSERT(sizeof(SND_NATIVE_CONTEXT) >= SND_CTX_OFF_IP + sizeof(ULONG_PTR),
                          snd_ctx_parity_ip_in_bounds);

#endif /* SND_USE_WINDOWS_SDK */

static snd_status_t WINAPI win_get_context(HANDLE thread, SND_THREAD_REGISTERS *out_regs) {
    SND_CHECK_NULL(thread, out_regs);

    CONTEXT ctx      = {0};
    ctx.ContextFlags = SND_CONTEXT_ARCH_FLAGS;

    if (!GetThreadContext(thread, &ctx)) {
        return SND_ERR_W32(SND_STATUS_THREAD_GET_CONTEXT_FAILED);
    }

    snd_context_to_portable(&ctx, out_regs);
    return SND_OK;
}

static snd_status_t WINAPI win_set_context(HANDLE thread, const SND_THREAD_REGISTERS *in_regs) {
    SND_CHECK_NULL(thread, in_regs);

    CONTEXT ctx      = {0};
    ctx.ContextFlags = SND_CONTEXT_ARCH_FLAGS;

    if (!GetThreadContext(thread, &ctx)) {
        return SND_ERR_W32(SND_STATUS_THREAD_GET_CONTEXT_FAILED);
    }

    snd_context_from_portable(in_regs, &ctx);

    if (!SetThreadContext(thread, &ctx)) {
        return SND_ERR_W32(SND_STATUS_THREAD_SET_CONTEXT_FAILED);
    }
    return SND_OK;
}

static snd_status_t WINAPI win_close_thread(HANDLE handle) {
    return CloseHandle(handle) ? SND_OK : SND_ERR(SND_STATUS_HANDLE_CLOSE_FAILED);
}

const snd_thread_api_t snd_thread_win = {.queue_apc      = win_queue_apc,
                                         .resume_thread  = win_resume_thread,
                                         .suspend_thread = win_suspend_thread,
                                         .get_context    = win_get_context,
                                         .set_context    = win_set_context,
                                         .close_handle   = win_close_thread};
