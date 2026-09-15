#ifndef SND_INJECTION_APC_ENGINE_H
#define SND_INJECTION_APC_ENGINE_H

#include <sindri/common/macros.h>
#include <sindri/injection/context.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Creates a target process in a suspended state.
 * @param ctx Initialized injection context with target_image_path and proc_api set.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the process API, or its
 * `create_process` callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the injection stage is not
 * `SND_INJ_STAGE_UNINITIALIZED`.
 * @retval Any error returned by `snd_process_api_t::create_process`.
 */
snd_status_t snd_inj_apc_create_target(snd_inj_ctx_t *ctx);

/**
 * @brief Allocates memory in the remote process for the payload.
 * @param ctx Context after SND_INJ_STAGE_TARGET_ACQUIRED.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the process API, or its
 * `alloc_remote` callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the injection stage is not
 * `SND_INJ_STAGE_TARGET_ACQUIRED`.
 * @retval Any error returned by `snd_process_api_t::alloc_remote`.
 */
snd_status_t snd_inj_apc_alloc_remote(snd_inj_ctx_t *ctx);

/**
 * @brief Writes the payload buffer into the allocated remote memory.
 * @param ctx Context after SND_INJ_STAGE_MEMORY_ALLOCATED.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the process API, its
 * `write_remote` callback, or the payload is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the injection stage is not
 * `SND_INJ_STAGE_MEMORY_ALLOCATED`.
 * @retval Any error returned by `snd_process_api_t::write_remote`.
 */
snd_status_t snd_inj_apc_write_payload(snd_inj_ctx_t *ctx);

/**
 * @brief Transitions remote memory protections from RW to RX.
 * @param ctx Context after SND_INJ_STAGE_PAYLOAD_WRITTEN.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the process API, or its
 * `protect_remote` callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the injection stage is not
 * `SND_INJ_STAGE_PAYLOAD_WRITTEN`.
 * @retval Any error returned by `snd_process_api_t::protect_remote`.
 */
snd_status_t snd_inj_apc_set_protections(snd_inj_ctx_t *ctx);

/**
 * @brief Queues an APC to the main thread and resumes it to execute the payload.
 * @param ctx Context after SND_INJ_STAGE_PROTECTIONS_SET.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the thread API, or one of its
 * required callbacks is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the injection stage is not
 * `SND_INJ_STAGE_PROTECTIONS_SET`.
 * @retval Any error returned by `snd_thread_api_t::queue_apc` or
 * `snd_thread_api_t::resume_thread`.
 */
snd_status_t snd_inj_apc_execute(snd_inj_ctx_t *ctx);

SND_END_EXTERN_C

#endif // SND_INJECTION_APC_ENGINE_H
