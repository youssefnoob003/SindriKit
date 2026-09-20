#include <sindri/injection/common/target.h>
#include <sindri/internal/nt/process.h>
#include <sindri/primitives/status.h>

snd_status_t snd_inj_open_target(snd_inj_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->proc_api, ctx->proc_api->open_process);

    if (ctx->stage != SND_INJ_STAGE_UNINITIALIZED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_UNINITIALIZED), snd_inj_stage_to_string(ctx->stage));
    }

    SND_TRY(ctx->proc_api->open_process(ctx->target_pid, SND_PROCESS_ALL_ACCESS, &ctx->target_process));
    ctx->stage = SND_INJ_STAGE_TARGET_ACQUIRED;
    return SND_OK;
}

snd_status_t snd_inj_create_suspended_target(snd_inj_ctx_t *ctx) {
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
    ctx->owns_target_process = TRUE;
    ctx->stage               = SND_INJ_STAGE_TARGET_ACQUIRED;
    return SND_OK;
}
