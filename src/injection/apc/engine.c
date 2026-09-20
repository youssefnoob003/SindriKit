#include <sindri/injection/apc/engine.h>
#include <sindri/primitives/status.h>

snd_status_t snd_inj_apc_execute(snd_inj_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->thread_api, ctx->thread_api->queue_apc, ctx->thread_api->resume_thread);
    if (ctx->stage != SND_INJ_STAGE_PROTECTIONS_SET) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_PROTECTIONS_SET), snd_inj_stage_to_string(ctx->stage));
    }
    PVOID entry_point = ctx->remote_entry_point ? ctx->remote_entry_point : ctx->remote_base;
    SND_TRY(ctx->thread_api->queue_apc(ctx->remote_thread, entry_point, ctx->remote_arg));
    SND_TRY(ctx->thread_api->resume_thread(ctx->remote_thread));
    ctx->stage = SND_INJ_STAGE_EXECUTED;
    /* Execution has been handed off successfully. Cleanup may close our
     * handles, but must not terminate the target after its thread resumes.
     * Keep ownership set until this point so pre-execution failures still
     * terminate targets created by this chain. */
    ctx->owns_target_process = FALSE;
    return SND_OK;
}
