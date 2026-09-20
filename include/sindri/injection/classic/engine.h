#ifndef SND_INJECTION_CLASSIC_ENGINE_H
#define SND_INJECTION_CLASSIC_ENGINE_H

#include <sindri/common/macros.h>
#include <sindri/injection/common/context.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

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
