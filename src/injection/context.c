#include <sindri/common/debug.h>
#include <sindri/injection/context.h>

const char *snd_inj_stage_to_string(snd_inj_stage_t stage) {
#if SND_DEBUG
    switch (stage) {
    case SND_INJ_STAGE_UNINITIALIZED:
        return "UNINITIALIZED";
    case SND_INJ_STAGE_TARGET_ACQUIRED:
        return "TARGET_ACQUIRED";
    case SND_INJ_STAGE_MEMORY_ALLOCATED:
        return "MEMORY_ALLOCATED";
    case SND_INJ_STAGE_PAYLOAD_WRITTEN:
        return "PAYLOAD_WRITTEN";
    case SND_INJ_STAGE_PROTECTIONS_SET:
        return "PROTECTIONS_SET";
    case SND_INJ_STAGE_EXECUTED:
        return "EXECUTED";
    default:
        return "UNKNOWN";
    }
#else
    (void)stage;
    return "";
#endif
}

void snd_inj_cleanup(snd_inj_ctx_t *ctx) {
    if (!ctx || !ctx->proc_api)
        return;

    if (ctx->remote_thread && ctx->proc_api->close_handle) {
        ctx->proc_api->close_handle(ctx->remote_thread);
        ctx->remote_thread = NULL;
    }

    if (ctx->target_process && ctx->proc_api->close_handle) {
        ctx->proc_api->close_handle(ctx->target_process);
        ctx->target_process = NULL;
    }

    ctx->remote_base        = NULL;
    ctx->remote_entry_point = NULL;
    ctx->remote_size        = 0;
    ctx->stage              = SND_INJ_STAGE_UNINITIALIZED;
}
