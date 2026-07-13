#include <sindri/common/macros.h>
#include <sindri/injection/classic/engine.h>
#include <sindri/injection/context.h>
#include <windows.h>

#ifndef PROCESS_ALL_ACCESS
#define PROCESS_ALL_ACCESS 0x001FFFFF
#endif

snd_status_t snd_inj_classic_open_target(snd_inj_ctx_t *ctx) {
    if (!ctx || !ctx->proc_api || !ctx->proc_api->open_process)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    if (ctx->stage != SND_INJ_STAGE_UNINITIALIZED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE_SEQUENCE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_UNINITIALIZED), snd_inj_stage_to_string(ctx->stage));
    }

    if (ctx->target_pid == 0) {
        return SND_ERR_CTX(SND_STATUS_INVALID_INJECTION_TARGET, "target_pid is 0");
    }

    snd_status_t status = ctx->proc_api->open_process(ctx->target_pid, PROCESS_ALL_ACCESS, &ctx->target_process);
    if (SND_FAILED(status))
        return status;

    ctx->stage = SND_INJ_STAGE_TARGET_ACQUIRED;
    return SND_OK;
}

snd_status_t snd_inj_classic_alloc_remote(snd_inj_ctx_t *ctx) {
    if (!ctx || !ctx->proc_api || !ctx->proc_api->alloc_remote)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    if (ctx->stage != SND_INJ_STAGE_TARGET_ACQUIRED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE_SEQUENCE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_TARGET_ACQUIRED), snd_inj_stage_to_string(ctx->stage));
    }

    if (ctx->payload == NULL || ctx->payload->data == NULL || ctx->payload->size == 0) {
        return SND_ERR_CTX(SND_STATUS_INVALID_PAYLOAD, "Payload buffer is empty or NULL");
    }

    ctx->remote_size = ctx->payload->size;

    snd_status_t status = ctx->proc_api->alloc_remote(ctx->target_process, ctx->remote_size, MEM_COMMIT | MEM_RESERVE,
                                                      PAGE_READWRITE, &ctx->remote_base);
    if (SND_FAILED(status))
        return status;

    ctx->stage = SND_INJ_STAGE_MEMORY_ALLOCATED;
    return SND_OK;
}

snd_status_t snd_inj_classic_write_payload(snd_inj_ctx_t *ctx) {
    if (!ctx || !ctx->proc_api || !ctx->proc_api->write_remote)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    if (ctx->stage != SND_INJ_STAGE_MEMORY_ALLOCATED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE_SEQUENCE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_MEMORY_ALLOCATED),
                           snd_inj_stage_to_string(ctx->stage));
    }

    SIZE_T       written = 0;
    snd_status_t status  = ctx->proc_api->write_remote(ctx->target_process, ctx->remote_base, ctx->payload->data,
                                                       ctx->remote_size, &written);
    if (SND_FAILED(status))
        return status;

    ctx->stage = SND_INJ_STAGE_PAYLOAD_WRITTEN;
    return SND_OK;
}

snd_status_t snd_inj_classic_set_protections(snd_inj_ctx_t *ctx) {
    if (!ctx || !ctx->proc_api || !ctx->proc_api->protect_remote)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    if (ctx->stage != SND_INJ_STAGE_PAYLOAD_WRITTEN) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE_SEQUENCE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_PAYLOAD_WRITTEN), snd_inj_stage_to_string(ctx->stage));
    }

    DWORD        old_protect = 0;
    snd_status_t status      = ctx->proc_api->protect_remote(ctx->target_process, ctx->remote_base, ctx->remote_size,
                                                             PAGE_EXECUTE_READ, &old_protect);
    if (SND_FAILED(status))
        return status;

    ctx->stage = SND_INJ_STAGE_PROTECTIONS_SET;
    return SND_OK;
}

snd_status_t snd_inj_classic_execute(snd_inj_ctx_t *ctx) {
    if (!ctx || !ctx->proc_api || !ctx->proc_api->create_remote_thread)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    if (ctx->stage != SND_INJ_STAGE_PROTECTIONS_SET) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE_SEQUENCE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_PROTECTIONS_SET), snd_inj_stage_to_string(ctx->stage));
    }

    PVOID        start_address = ctx->remote_entry_point ? ctx->remote_entry_point : ctx->remote_base;
    snd_status_t status =
        ctx->proc_api->create_remote_thread(ctx->target_process, start_address, ctx->remote_arg, &ctx->remote_thread);
    if (SND_FAILED(status))
        return status;

    ctx->stage = SND_INJ_STAGE_EXECUTED;
    return SND_OK;
}
