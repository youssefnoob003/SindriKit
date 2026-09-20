#ifndef SND_INJECTION_COMMON_TARGET_H
#define SND_INJECTION_COMMON_TARGET_H

#include <sindri/common/macros.h>
#include <sindri/injection/common/context.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Opens an existing target process by PID.
 * @param ctx Context with `target_pid` and `proc_api` initialized. The target
 * process is caller-owned because it was not created by this operation.
 * @retval SND_OK On success; the stage becomes `TARGET_ACQUIRED`.
 * @retval SND_STATUS_NULL_POINTER If a required context or callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the context is not uninitialized.
 * @retval Any error returned by `open_process`.
 */
snd_status_t snd_inj_open_target(snd_inj_ctx_t *ctx);

/**
 * @brief Creates a suspended target process and stores its initial thread.
 * @param ctx Context with `target_image_path` and `proc_api` initialized. The
 * created target is marked for best-effort cleanup until execution succeeds.
 * @retval SND_OK On success; the stage becomes `TARGET_ACQUIRED`.
 * @retval SND_STATUS_NULL_POINTER If a required context or callback is NULL.
 * @retval SND_STATUS_CORRUPTED_STAGE If the image path is missing.
 * @retval Any error returned by `create_process`.
 */
snd_status_t snd_inj_create_suspended_target(snd_inj_ctx_t *ctx);

SND_END_EXTERN_C

#endif // SND_INJECTION_COMMON_TARGET_H
