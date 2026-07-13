#ifndef SND_LOADERS_REFLECTIVE_CHAIN_H
#define SND_LOADERS_REFLECTIVE_CHAIN_H

#include <sindri/common/macros.h>
#include <sindri/loaders/pe/engine.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Executes the entire allocation and fixup chain.
 *
 * @param ctx Initialized loader context.
 * @return SND_OK on success, otherwise the failing stage status.
 * @note Wraps compatibility check, copy, relocation, imports, protections, and
 * TLS.
 */
snd_status_t snd_ldr_pe_prepare_image(snd_ldr_pe_ctx_t *ctx);

/**
 * @brief Executes the loaded image entry point.
 *
 * @param ctx Prepared loader context.
 * @return SND_OK on success, otherwise execution error.
 */
snd_status_t snd_ldr_pe_execute_image(snd_ldr_pe_ctx_t *ctx);

/**
 * @brief Cleans up and detaches the pe image state.
 *
 * @param ctx Reflective loader context.
 */
void snd_ldr_pe_detach_image(snd_ldr_pe_ctx_t *ctx);

SND_END_EXTERN_C

#endif // SND_LOADERS_REFLECTIVE_CHAIN_H
