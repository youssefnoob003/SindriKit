#ifndef SND_INJECTION_COMMON_CONTEXT_H
#define SND_INJECTION_COMMON_CONTEXT_H

#include <sindri/common/buffer.h>
#include <sindri/common/debug.h>
#include <sindri/common/macros.h>
#include <sindri/internal/windows/types.h>
#include <sindri/primitives/os_api.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Stages enforced by the injection state machine.
 *
 * Each engine operation accepts only the stage produced by its predecessor.
 * `SND_INJ_STAGE_CONTEXT_APPLIED` is specific to thread hijacking; the other
 * techniques transition directly from `SND_INJ_STAGE_PROTECTIONS_SET` to
 * `SND_INJ_STAGE_EXECUTED`.
 */
typedef enum {
    SND_INJ_STAGE_UNINITIALIZED = 0, /**< No target or remote allocation is active. */
    SND_INJ_STAGE_TARGET_ACQUIRED,   /**< The target process handle is valid. */
    SND_INJ_STAGE_MEMORY_ALLOCATED,  /**< The remote staging region is allocated. */
    SND_INJ_STAGE_PAYLOAD_WRITTEN,   /**< The payload bytes are present remotely. */
    SND_INJ_STAGE_PROTECTIONS_SET,   /**< The remote region has execute-read protection. */
    SND_INJ_STAGE_CONTEXT_APPLIED,   /**< Hijack context is applied; resume is pending. */
    SND_INJ_STAGE_EXECUTED,          /**< Execution has been handed off successfully. */
} snd_inj_stage_t;

/**
 * @brief Policy used by hijack execution when the payload returns.
 *
 * The policy is ignored by classic and APC execution chains. Graceful return
 * resolution is performed by the hijack engine and is not a thread API
 * capability.
 */
typedef enum {
    SND_INJ_RETURN_NONE = 0, /**< Payload must not return. */
    SND_INJ_RETURN_GRACEFUL, /**< Engine resolves a target-compatible thread-exit address. */
} snd_inj_return_policy_t;

SND_SHUFFLE_START
/**
 * @brief Shared state and injected capabilities for an injection operation.
 *
 * Callers normally zero-initialize this structure, set the fields required by
 * the selected chain, and call `snd_inj_cleanup` after the operation. The
 * context does not own the caller-provided payload buffer, loader contexts, or
 * target image path.
 */
typedef struct _snd_inj_ctx_t {
    DWORD  target_pid;          /**< Existing target PID; used by classic chains. */
    HANDLE target_process;      /**< Acquired or created target process handle. */
    PVOID  remote_base;         /**< Base address of the remote staging region. */
    PVOID  remote_entry_point;  /**< Payload entry point; falls back to `remote_base`. */
    SIZE_T remote_size;         /**< Size of the remote staging region. */
    HANDLE remote_thread;       /**< Remote-created or suspended initial thread. */
    PVOID  remote_arg;          /**< Remote BOF argument address, when applicable. */
    BOOL   owns_target_process; /**< Cleanup may terminate a target created by the chain. */

    snd_inj_stage_t stage; /**< Current stage of the injection state machine. */

    const snd_buffer_t      *payload;           /**< Caller-owned raw or prepared payload. */
    const snd_process_api_t *proc_api;          /**< Remote process capability table. */
    const snd_thread_api_t  *thread_api;        /**< APC/context capability table. */
    const wchar_t           *target_image_path; /**< Suspended target image path. */
    snd_inj_return_policy_t  return_policy;     /**< Hijack-only payload return policy. */
} snd_inj_ctx_t;
SND_SHUFFLE_END

/**
 * @brief Converts an injection stage to a debug string.
 * @param stage Stage value to convert.
 * @return Pointer to a constant string, or an empty string when debug support
 * is disabled.
 */
const char *snd_inj_stage_to_string(snd_inj_stage_t stage);

SND_END_EXTERN_C

#endif // SND_INJECTION_COMMON_CONTEXT_H
