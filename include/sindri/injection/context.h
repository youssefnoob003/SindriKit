#ifndef SND_INJECTION_CONTEXT_H
#define SND_INJECTION_CONTEXT_H

#include <sindri/common/buffer.h>
#include <sindri/common/debug.h>
#include <sindri/common/macros.h>
#include <sindri/primitives/os_api.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

typedef enum {
    SND_INJ_STAGE_UNINITIALIZED = 0,
    SND_INJ_STAGE_TARGET_ACQUIRED,
    SND_INJ_STAGE_MEMORY_ALLOCATED,
    SND_INJ_STAGE_PAYLOAD_WRITTEN,
    SND_INJ_STAGE_PROTECTIONS_SET,
    SND_INJ_STAGE_EXECUTED,
} snd_inj_stage_t;

typedef struct _snd_inj_ctx_t {
    DWORD  target_pid;
    HANDLE target_process;
    PVOID  remote_base;
    PVOID  remote_entry_point;
    SIZE_T remote_size;
    HANDLE remote_thread;
    PVOID  remote_arg;

    snd_inj_stage_t stage;

    const snd_buffer_t      *payload;
    const snd_process_api_t *proc_api;
} snd_inj_ctx_t;

/**
 * @brief Converts an injection stage enum to a human-readable string.
 */
const char *snd_inj_stage_to_string(snd_inj_stage_t stage);

/**
 * @brief Cleans up injection context: closes handles, resets state.
 */
void snd_inj_cleanup(snd_inj_ctx_t *ctx);

SND_END_EXTERN_C

#endif // SND_INJECTION_CONTEXT_H
