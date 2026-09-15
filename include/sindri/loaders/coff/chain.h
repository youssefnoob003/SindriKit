#ifndef SND_LOADERS_COFF_CHAIN_H
#define SND_LOADERS_COFF_CHAIN_H

#include <sindri/common/buffer.h>
#include <sindri/common/macros.h>
#include <sindri/internal/windows/types.h>
#include <sindri/loaders/coff/engine.h>
#include <sindri/status/core.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Orchestrates the complete loading of a COFF object file.
 *
 * This function handles parsing the COFF from the source buffer, allocating
 * and mapping the sections, resolving external symbols using the provided callback,
 * applying relocations, and finally applying memory protections.
 *
 * The caller must pre-initialize `ctx_out->raw_source`, `ctx_out->mem_api`,
 * and `ctx_out->mod_api` before calling this function.
 *
 * @param ctx_out Loader context pre-initialized with `raw_source`, `mem_api`, and `mod_api`.
 *                On success, contains the fully loaded COFF execution state.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx_out is NULL.
 * @retval Any error returned by `snd_coff_parse`,
 * `snd_ldr_coff_allocate_and_copy_sections`,
 * `snd_ldr_coff_resolve_symbols`, `snd_ldr_coff_apply_relocations`, or
 * `snd_ldr_coff_apply_memory_protections`.
 * @note If a failure occurs after memory allocation, the memory is
 * automatically freed.
 */
snd_status_t snd_ldr_coff_load(snd_ldr_coff_ctx_t *ctx_out);

/**
 * @brief Executes the loaded COFF image.
 *
 * @param ctx Loader context containing the loaded COFF image.
 * @param entry_name Name of the entry point symbol to execute.
 * @param bof_args Arguments to pass to the entry point.
 * @param bof_arg_len Length of the bof_args buffer.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx is NULL.
 * @retval SND_STATUS_ARCH_MISMATCH If the COFF image architecture is
 * incompatible.
 * @retval Any error returned by `snd_coff_find_symbol_by_name`.
 */
snd_status_t snd_ldr_coff_execute_image(snd_ldr_coff_ctx_t *ctx, const char *entry_name, char *bof_args,
                                        int bof_arg_len);

SND_END_EXTERN_C

#endif // SND_LOADERS_COFF_CHAIN_H
