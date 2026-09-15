#ifndef SND_LOADERS_REFLECTIVE_CHAIN_H
#define SND_LOADERS_REFLECTIVE_CHAIN_H

#include <sindri/common/macros.h>
#include <sindri/loaders/pe/engine.h>
#include <sindri/status/core.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Executes the entire allocation and fixup chain.
 *
 * @param ctx Initialized loader context.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx or its required fields are
 * NULL.
 * @retval SND_STATUS_ARCH_MISMATCH If the image architecture is incompatible.
 * @retval Any error returned by `snd_pe_parse`,
 * `snd_ldr_pe_allocate_and_copy_image`, `snd_ldr_pe_apply_relocations`,
 * `snd_ldr_pe_resolve_imports`, or `snd_ldr_pe_apply_memory_protections`.
 * @note Wraps compatibility check, copy, relocation, imports, protections, and
 * TLS.
 */
snd_status_t snd_ldr_pe_prepare_image(snd_ldr_pe_ctx_t *ctx);

/**
 * @brief Executes the loaded image entry point.
 *
 * @param ctx Prepared loader context.
 * @retval SND_OK On success.
 * @retval SND_STATUS_INVALID_STAGE If the loader stage is invalid.
 * @retval SND_STATUS_LOCAL_EXECUTION_BLOCKED If local execution is disabled.
 * @retval SND_STATUS_DLL_INITIALIZATION_FAILED If DLL initialization fails.
 * @retval SND_STATUS_IMAGE_ENTRY_POINT_MISSING If an executable entry point is
 * absent.
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
