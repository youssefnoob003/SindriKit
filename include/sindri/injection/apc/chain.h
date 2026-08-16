#ifndef SND_INJECTION_APC_CHAIN_H
#define SND_INJECTION_APC_CHAIN_H

#include <sindri/common/macros.h>
#include <sindri/injection/apc/engine.h>
#include <sindri/injection/context.h>

SND_BEGIN_EXTERN_C

typedef struct _snd_ldr_pe_ctx   snd_ldr_pe_ctx_t;
typedef struct _snd_ldr_coff_ctx snd_ldr_coff_ctx_t;

/**
 * @brief Executes the full Early Bird APC injection pipeline.
 *
 * Runs: create_target -> alloc_remote -> write_payload -> set_protections -> execute.
 *
 * @param ctx Initialized injection context with target_image_path, payload, proc_api, and thread_api set.
 * @return SND_OK on success, otherwise the failing stage status.
 */
snd_status_t snd_inj_apc_shell(snd_inj_ctx_t *ctx);

/**
 * @brief High-level orchestrator that links a PE loader context and an APC injection context.
 *
 * @param ldr_ctx Initialized PE loader context with raw_source, mem_api, and mod_api set.
 * @param inj_ctx Initialized injection context with target_image_path, proc_api, and thread_api set.
 * @return SND_OK on success, otherwise the failing stage status.
 */
snd_status_t snd_inj_apc_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx);

/**
 * @brief High-level orchestrator that links a COFF loader context and an APC injection context.
 *
 * @param ldr_ctx Initialized COFF loader context with raw_source, mem_api, and mod_api set.
 * @param inj_ctx Initialized injection context with target_image_path, proc_api, and thread_api set.
 * @param entry_point The name of the BOF entry point to execute (e.g., "go").
 * @param args The BOF arguments buffer.
 * @param arg_len The length of the BOF arguments buffer.
 * @return SND_OK on success, otherwise the failing stage status.
 */
snd_status_t snd_inj_apc_coff(snd_ldr_coff_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx, const char *entry_point,
                              void *args, int arg_len);

SND_END_EXTERN_C

#endif // SND_INJECTION_APC_CHAIN_H
