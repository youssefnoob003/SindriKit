#include <sindri/injection/common/cleanup.h>
#include <sindri/internal/windows/constants.h>

void snd_inj_cleanup(snd_inj_ctx_t *ctx) {
    if (!ctx || !ctx->proc_api)
        return;

    if (ctx->remote_thread) {
        if (ctx->thread_api && ctx->thread_api->close_handle) {
            ctx->thread_api->close_handle(ctx->remote_thread);
        } else if (ctx->proc_api->close_handle) {
            ctx->proc_api->close_handle(ctx->remote_thread);
        }
        ctx->remote_thread = NULL;
    }

    if (ctx->remote_base && ctx->stage < SND_INJ_STAGE_EXECUTED && ctx->proc_api->free_remote) {
        ctx->proc_api->free_remote(ctx->target_process, ctx->remote_base, 0, SND_MEM_RELEASE);
    }

    if (ctx->target_process && ctx->owns_target_process && ctx->proc_api->terminate_process) {
        ctx->proc_api->terminate_process(ctx->target_process, SND_INJ_CLEANUP_EXIT_CODE);
    }

    if (ctx->target_process && ctx->proc_api->close_handle) {
        ctx->proc_api->close_handle(ctx->target_process);
        ctx->target_process = NULL;
    }

    ctx->remote_base         = NULL;
    ctx->remote_entry_point  = NULL;
    ctx->remote_size         = 0;
    ctx->remote_arg          = NULL;
    ctx->payload             = NULL;
    ctx->owns_target_process = FALSE;
    ctx->stage               = SND_INJ_STAGE_UNINITIALIZED;
}
