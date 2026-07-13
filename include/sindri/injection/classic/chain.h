#ifndef SND_INJECTION_CLASSIC_CHAIN_H
#define SND_INJECTION_CLASSIC_CHAIN_H

#include <sindri/common/macros.h>
#include <sindri/injection/classic/engine.h>
#include <sindri/injection/context.h>

SND_BEGIN_EXTERN_C

// Forward declare the loader contexts so we don't have to include the loader module
typedef struct _snd_ldr_pe_ctx   snd_ldr_pe_ctx_t;
typedef struct _snd_ldr_coff_ctx snd_ldr_coff_ctx_t;

/**
 * @brief Executes the full classic injection pipeline.
 *
 * Runs: open_target -> alloc_remote -> write_payload -> set_protections -> execute.
 *
 * @param ctx Initialized injection context with target_pid, payload, and proc_api set.
 * @return SND_OK on success, otherwise the failing stage status.
 */
snd_status_t snd_inj_classic_shell(snd_inj_ctx_t *ctx);

/**
 * @brief High-level orchestrator that links a loader context and an injection context.
 *
 * The user initializes both contexts with their desired API backends, and this
 * chain handles the inter-context data marshaling and execution flow.
 *
 * @param ldr_ctx Initialized PE loader context with raw_source, mem_api, and mod_api set.
 * @param inj_ctx Initialized injection context with target_pid, and proc_api set.
 * @return SND_OK on success, otherwise the failing stage status.
 */
snd_status_t snd_inj_classic_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx);

/**
 * @brief High-level orchestrator that links a COFF loader context and an injection context.
 *
 * @param ldr_ctx Initialized COFF loader context with raw_source, mem_api, and mod_api set.
 * @param inj_ctx Initialized injection context with target_pid, and proc_api set.
 * @param entry_point The name of the BOF entry point to execute (e.g., "go").
 * @param args The BOF arguments buffer.
 * @param arg_len The length of the BOF arguments buffer.
 * @return SND_OK on success, otherwise the failing stage status.
 */
snd_status_t snd_inj_classic_coff(snd_ldr_coff_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx, const char *entry_point,
                                  void *args, int arg_len);

SND_END_EXTERN_C

#endif // SND_INJECTION_CLASSIC_CHAIN_H
