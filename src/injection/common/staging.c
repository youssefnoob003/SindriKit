#include <sindri/injection/common/staging.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/primitives/status.h>

snd_status_t snd_inj_alloc_remote_size(snd_inj_ctx_t *ctx, SIZE_T size) {
    SND_CHECK_NULL(ctx, ctx->proc_api, ctx->proc_api->alloc_remote);
    if (ctx->stage != SND_INJ_STAGE_TARGET_ACQUIRED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_TARGET_ACQUIRED), snd_inj_stage_to_string(ctx->stage));
    }
    if (size == 0) {
        return SND_ERR_CTX(SND_STATUS_CORRUPTED_STAGE, "Remote allocation size is zero.");
    }
    ctx->remote_size = size;
    SND_TRY(ctx->proc_api->alloc_remote(ctx->target_process, size, SND_MEM_COMMIT | SND_MEM_RESERVE, SND_PAGE_READWRITE,
                                        &ctx->remote_base));
    ctx->stage = SND_INJ_STAGE_MEMORY_ALLOCATED;
    return SND_OK;
}

snd_status_t snd_inj_alloc_remote(snd_inj_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->payload, ctx->payload->size);
    return snd_inj_alloc_remote_size(ctx, ctx->payload->size);
}

snd_status_t snd_inj_write_payload(snd_inj_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->proc_api, ctx->proc_api->write_remote, ctx->payload);
    SND_CHECK_NULL(ctx->payload->data, ctx->payload->size);
    if (ctx->stage != SND_INJ_STAGE_MEMORY_ALLOCATED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_MEMORY_ALLOCATED),
                           snd_inj_stage_to_string(ctx->stage));
    }
    SIZE_T written = 0;
    SND_TRY(ctx->proc_api->write_remote(ctx->target_process, ctx->remote_base, ctx->payload->data, ctx->payload->size,
                                        &written));
    if (written != ctx->payload->size) {
        return SND_ERR_CTX(SND_STATUS_PROCESS_REMOTE_WRITE_FAILED, "Incomplete payload write.");
    }
    ctx->stage = SND_INJ_STAGE_PAYLOAD_WRITTEN;
    return SND_OK;
}

snd_status_t snd_inj_set_protections(snd_inj_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->proc_api, ctx->proc_api->protect_remote);
    if (ctx->stage != SND_INJ_STAGE_PAYLOAD_WRITTEN) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_PAYLOAD_WRITTEN), snd_inj_stage_to_string(ctx->stage));
    }
    DWORD old_protect = 0;
    SND_TRY(ctx->proc_api->protect_remote(ctx->target_process, ctx->remote_base, ctx->remote_size,
                                          SND_PAGE_EXECUTE_READ, &old_protect));
    ctx->stage = SND_INJ_STAGE_PROTECTIONS_SET;
    return SND_OK;
}
