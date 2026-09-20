#include <sindri/injection/classic/engine.h>
#include <sindri/primitives/status.h>

snd_status_t snd_inj_classic_execute(snd_inj_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->proc_api, ctx->proc_api->create_remote_thread);
    if (ctx->stage != SND_INJ_STAGE_PROTECTIONS_SET) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_PROTECTIONS_SET), snd_inj_stage_to_string(ctx->stage));
    }
    PVOID start_address = ctx->remote_entry_point ? ctx->remote_entry_point : ctx->remote_base;
    SND_TRY(
        ctx->proc_api->create_remote_thread(ctx->target_process, start_address, ctx->remote_arg, &ctx->remote_thread));
    ctx->stage = SND_INJ_STAGE_EXECUTED;
    return SND_OK;
}
