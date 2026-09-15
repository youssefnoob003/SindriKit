#ifndef SND_INJECTION_CLASSIC_ENGINE_H
#define SND_INJECTION_CLASSIC_ENGINE_H

#include <sindri/common/macros.h>
#include <sindri/injection/context.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Opens a handle to the target process.
 * @param ctx Initialized injection context with target_pid and proc_api set.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the process API, or its
 * `open_process` callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the injection stage is not
 * `SND_INJ_STAGE_UNINITIALIZED`.
 * @retval Any error returned by `snd_process_api_t::open_process`.
 */
snd_status_t snd_inj_classic_open_target(snd_inj_ctx_t *ctx);

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
snd_status_t snd_inj_classic_alloc_remote(snd_inj_ctx_t *ctx);

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
snd_status_t snd_inj_classic_write_payload(snd_inj_ctx_t *ctx);

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
snd_status_t snd_inj_classic_set_protections(snd_inj_ctx_t *ctx);

/**
 * @brief Creates a remote thread at the payload base to execute the shellcode.
 * @param ctx Context after SND_INJ_STAGE_PROTECTIONS_SET.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the process API, or its
 * `create_remote_thread` callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the injection stage is not
 * `SND_INJ_STAGE_PROTECTIONS_SET`.
 * @retval Any error returned by `snd_process_api_t::create_remote_thread`.
 */
snd_status_t snd_inj_classic_execute(snd_inj_ctx_t *ctx);

SND_END_EXTERN_C

#endif // SND_INJECTION_CLASSIC_ENGINE_H
