#include <sindri/common/memory.h>
#include <sindri/injection/hijack/engine.h>
#include <sindri/parsers/env/ntdll.h>
#include <sindri/primitives/status.h>
#include <sindri_hashes.h>

#if defined(_WIN64)

/* x64: the payload observes the state left by a CALL — RSP is 16-byte aligned
 * with the return address in the word below it (RSP % 16 == 8 at entry). Only
 * that return slot is written; arguments travel in RCX/RDX. */
static snd_status_t snd_hijack_prepare_frame_x64(const SND_THREAD_REGISTERS *live, PVOID entry, PVOID return_thunk,
                                                 ULONG_PTR arg1, ULONG_PTR arg2, snd_inj_entry_frame_t *out) {
    out->registers.ip     = (ULONG_PTR)entry;
    out->registers.sp     = SND_ALIGN_UP(live->sp, SND_INJ_X64_STACK_ALIGNMENT) - sizeof(PVOID);
    out->registers.cx     = arg1;
    out->registers.dx     = arg2;
    out->registers.rflags = live->rflags | SND_EFLAGS_IF | SND_EFLAGS_RESERVED1;
    out->stack[0]         = (ULONG_PTR)return_thunk;
    out->stack_size       = return_thunk ? sizeof(PVOID) : 0;
    return SND_OK;
}

#else

/* x86 cdecl: arguments are pushed right-to-left, so the payload reads
 * [ESP] = return address, [ESP+4] = arg1, [ESP+8] = arg2. ESP has no alignment
 * requirement. The frame is written only when it carries content. */
static snd_status_t snd_hijack_prepare_frame_x86(const SND_THREAD_REGISTERS *live, PVOID entry, PVOID return_thunk,
                                                 ULONG_PTR arg1, ULONG_PTR arg2, snd_inj_entry_frame_t *out) {
    if (return_thunk || arg1 || arg2) {
        if (live->sp < (ULONG_PTR)sizeof(out->stack)) {
            return SND_ERR(SND_STATUS_INVALID_PARAMETERS_COMBINATION);
        }
        out->stack[0]     = (ULONG_PTR)return_thunk;
        out->stack[1]     = arg1;
        out->stack[2]     = arg2;
        out->stack_size   = sizeof(out->stack);
        out->registers.sp = live->sp - (ULONG_PTR)sizeof(out->stack);
    } else {
        out->stack_size   = 0;
        out->registers.sp = live->sp;
    }

    out->registers.ip     = (ULONG_PTR)entry;
    out->registers.cx     = 0;
    out->registers.dx     = 0;
    out->registers.rflags = live->rflags | SND_EFLAGS_IF | SND_EFLAGS_RESERVED1;
    return SND_OK;
}

#endif

static snd_status_t snd_hijack_write_entry_frame(const snd_inj_ctx_t *ctx, const snd_inj_entry_frame_t *frame) {
    if (frame->stack_size == 0) {
        return SND_OK;
    }

    SND_CHECK_NULL(ctx->proc_api, ctx->proc_api->write_remote);

    SIZE_T written = 0;
    SND_TRY(ctx->proc_api->write_remote(ctx->target_process, (PVOID)frame->registers.sp, frame->stack,
                                        frame->stack_size, &written));
    if (written != frame->stack_size) {
        return SND_ERR_CTX(SND_STATUS_PROCESS_REMOTE_WRITE_FAILED, "Incomplete entry frame write.");
    }
    return SND_OK;
}

static snd_status_t snd_hijack_resolve_return_thunk(snd_inj_ctx_t *ctx, PVOID *out_thunk) {
    switch (ctx->return_policy) {
    case SND_INJ_RETURN_NONE:
        *out_thunk = NULL;
        return SND_OK;
    case SND_INJ_RETURN_GRACEFUL:
        return snd_ntdll_get_active_export(SND_HASH_RTLEXITUSERTHREAD, out_thunk);
    default:
        return SND_ERR_CTX(SND_STATUS_INVALID_PARAMETERS_COMBINATION, "Unknown return_policy %d", ctx->return_policy);
    }
}

snd_status_t snd_inj_hijack_prepare_frame(const SND_THREAD_REGISTERS *live, PVOID entry, PVOID return_thunk,
                                          ULONG_PTR arg1, ULONG_PTR arg2, snd_inj_entry_frame_t *out) {
    SND_CHECK_NULL(live, out, entry);

#if defined(_WIN64)
    return snd_hijack_prepare_frame_x64(live, entry, return_thunk, arg1, arg2, out);
#else
    return snd_hijack_prepare_frame_x86(live, entry, return_thunk, arg1, arg2, out);
#endif
}

snd_status_t snd_inj_hijack_execute(snd_inj_ctx_t *ctx, ULONG_PTR arg1, ULONG_PTR arg2) {
    SND_CHECK_NULL(ctx, ctx->proc_api);
    SND_CHECK_NULL(ctx->thread_api, ctx->thread_api->get_context, ctx->thread_api->set_context);
    SND_CHECK_NULL(ctx->thread_api->resume_thread);

    if (ctx->stage != SND_INJ_STAGE_PROTECTIONS_SET) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_PROTECTIONS_SET), snd_inj_stage_to_string(ctx->stage));
    }

    PVOID entry = ctx->remote_entry_point ? ctx->remote_entry_point : ctx->remote_base;
    SND_CHECK_NULL(entry);
    SND_CHECK_NULL(ctx->target_process, ctx->remote_thread);

    PVOID return_thunk = NULL;
    SND_TRY(snd_hijack_resolve_return_thunk(ctx, &return_thunk));

    SND_THREAD_REGISTERS live = {0};
    SND_TRY(ctx->thread_api->get_context(ctx->remote_thread, &live));

    snd_inj_entry_frame_t frame = {0};
    SND_TRY(snd_inj_hijack_prepare_frame(&live, entry, return_thunk, arg1, arg2, &frame));

    SND_TRY(snd_hijack_write_entry_frame(ctx, &frame));

    SND_TRY(ctx->thread_api->set_context(ctx->remote_thread, &frame.registers));
    ctx->stage = SND_INJ_STAGE_CONTEXT_APPLIED;
    SND_TRY(ctx->thread_api->resume_thread(ctx->remote_thread));

    ctx->stage               = SND_INJ_STAGE_EXECUTED;
    ctx->owns_target_process = FALSE;
    return SND_OK;
}
