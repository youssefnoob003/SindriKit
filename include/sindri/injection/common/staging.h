#ifndef SND_INJECTION_COMMON_STAGING_H
#define SND_INJECTION_COMMON_STAGING_H

#include <sindri/common/macros.h>
#include <sindri/injection/common/context.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Allocates remote staging memory sized to `ctx->payload`.
 * @param ctx Context after `SND_INJ_STAGE_TARGET_ACQUIRED` with a non-empty
 * payload and an `alloc_remote` callback.
 * @retval SND_OK On success; the stage becomes `SND_INJ_STAGE_MEMORY_ALLOCATED`.
 * @retval SND_STATUS_NULL_POINTER If a required context, payload, or callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the target has not been acquired.
 * @retval SND_STATUS_CORRUPTED_STAGE If the payload size is zero.
 * @retval Any error returned by `snd_process_api_t::alloc_remote`.
 */
snd_status_t snd_inj_alloc_remote(snd_inj_ctx_t *ctx);

/**
 * @brief Allocates a caller-specified remote staging size.
 * @param ctx Context after `SND_INJ_STAGE_TARGET_ACQUIRED`.
 * @param size Number of bytes to reserve and commit remotely; must be non-zero.
 * @retval SND_OK On success; the stage becomes `SND_INJ_STAGE_MEMORY_ALLOCATED`.
 * @retval SND_STATUS_NULL_POINTER If a required context or callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the target has not been acquired.
 * @retval SND_STATUS_CORRUPTED_STAGE If @p size is zero.
 * @retval Any error returned by `snd_process_api_t::alloc_remote`.
 */
snd_status_t snd_inj_alloc_remote_size(snd_inj_ctx_t *ctx, SIZE_T size);

/**
 * @brief Writes `ctx->payload` into the allocated remote region.
 * @param ctx Context after `SND_INJ_STAGE_MEMORY_ALLOCATED` with a non-empty
 * payload and a `write_remote` callback.
 * @retval SND_OK On success; the stage becomes `SND_INJ_STAGE_PAYLOAD_WRITTEN`.
 * @retval SND_STATUS_NULL_POINTER If a required context, payload, data pointer,
 * or callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If remote memory has not been allocated.
 * @retval SND_STATUS_PROCESS_REMOTE_WRITE_FAILED If the write is incomplete.
 * @retval Any error returned by `snd_process_api_t::write_remote`.
 */
snd_status_t snd_inj_write_payload(snd_inj_ctx_t *ctx);

/**
 * @brief Sets the staged remote region to execute-read protection.
 * @param ctx Context after `SND_INJ_STAGE_PAYLOAD_WRITTEN` with a
 * `protect_remote` callback.
 * @retval SND_OK On success; the stage becomes `SND_INJ_STAGE_PROTECTIONS_SET`.
 * @retval SND_STATUS_NULL_POINTER If a required context or callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the payload has not been written.
 * @retval Any error returned by `snd_process_api_t::protect_remote`.
 */
snd_status_t snd_inj_set_protections(snd_inj_ctx_t *ctx);

SND_END_EXTERN_C

#endif // SND_INJECTION_COMMON_STAGING_H
