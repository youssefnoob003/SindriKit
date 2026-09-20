#ifndef SND_INJECTION_APC_ENGINE_H
#define SND_INJECTION_APC_ENGINE_H

#include <sindri/common/macros.h>
#include <sindri/injection/common/context.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

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
