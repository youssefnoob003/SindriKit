#ifndef SND_INJECTION_CLASSIC_ENGINE_H
#define SND_INJECTION_CLASSIC_ENGINE_H

#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/injection/context.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Opens a handle to the target process.
 * @param ctx Initialized injection context with target_pid and proc_api set.
 * @return SND_OK on success, otherwise SND_STATUS_PROCESS_OPEN_FAILED.
 */
snd_status_t snd_inj_classic_open_target(snd_inj_ctx_t *ctx);

/**
 * @brief Allocates memory in the remote process for the payload.
 * @param ctx Context after SND_INJ_STAGE_TARGET_ACQUIRED.
 * @return SND_OK on success, otherwise SND_STATUS_REMOTE_ALLOC_FAILED.
 */
snd_status_t snd_inj_classic_alloc_remote(snd_inj_ctx_t *ctx);

/**
 * @brief Writes the payload buffer into the allocated remote memory.
 * @param ctx Context after SND_INJ_STAGE_MEMORY_ALLOCATED.
 * @return SND_OK on success, otherwise SND_STATUS_REMOTE_WRITE_FAILED.
 */
snd_status_t snd_inj_classic_write_payload(snd_inj_ctx_t *ctx);

/**
 * @brief Transitions remote memory protections from RW to RX.
 * @param ctx Context after SND_INJ_STAGE_PAYLOAD_WRITTEN.
 * @return SND_OK on success, otherwise SND_STATUS_REMOTE_PROTECT_FAILED.
 */
snd_status_t snd_inj_classic_set_protections(snd_inj_ctx_t *ctx);

/**
 * @brief Creates a remote thread at the payload base to execute the shellcode.
 * @param ctx Context after SND_INJ_STAGE_PROTECTIONS_SET.
 * @return SND_OK on success, otherwise SND_STATUS_THREAD_CREATE_FAILED.
 */
snd_status_t snd_inj_classic_execute(snd_inj_ctx_t *ctx);

SND_END_EXTERN_C

#endif // SND_INJECTION_CLASSIC_ENGINE_H
