#ifndef SND_INJECTION_APC_ENGINE_H
#define SND_INJECTION_APC_ENGINE_H

#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/injection/context.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Creates a target process in a suspended state.
 * @param ctx Initialized injection context with target_image_path and proc_api set.
 * @return SND_OK on success, otherwise SND_STATUS_PROCESS_OPEN_FAILED.
 */
snd_status_t snd_inj_apc_create_target(snd_inj_ctx_t *ctx);

/**
 * @brief Allocates memory in the remote process for the payload.
 * @param ctx Context after SND_INJ_STAGE_TARGET_ACQUIRED.
 * @return SND_OK on success, otherwise SND_STATUS_REMOTE_ALLOC_FAILED.
 */
snd_status_t snd_inj_apc_alloc_remote(snd_inj_ctx_t *ctx);

/**
 * @brief Writes the payload buffer into the allocated remote memory.
 * @param ctx Context after SND_INJ_STAGE_MEMORY_ALLOCATED.
 * @return SND_OK on success, otherwise SND_STATUS_REMOTE_WRITE_FAILED.
 */
snd_status_t snd_inj_apc_write_payload(snd_inj_ctx_t *ctx);

/**
 * @brief Transitions remote memory protections from RW to RX.
 * @param ctx Context after SND_INJ_STAGE_PAYLOAD_WRITTEN.
 * @return SND_OK on success, otherwise SND_STATUS_REMOTE_PROTECT_FAILED.
 */
snd_status_t snd_inj_apc_set_protections(snd_inj_ctx_t *ctx);

/**
 * @brief Queues an APC to the main thread and resumes it to execute the payload.
 * @param ctx Context after SND_INJ_STAGE_PROTECTIONS_SET.
 * @return SND_OK on success, otherwise SND_STATUS_APC_QUEUE_FAILED.
 */
snd_status_t snd_inj_apc_execute(snd_inj_ctx_t *ctx);

SND_END_EXTERN_C

#endif // SND_INJECTION_APC_ENGINE_H
