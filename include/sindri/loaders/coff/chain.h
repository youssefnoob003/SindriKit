#ifndef SND_LOADERS_COFF_CHAIN_H
#define SND_LOADERS_COFF_CHAIN_H

#include <sindri/common/buffer.h>
#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/loaders/coff/engine.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Orchestrates the complete loading of a COFF object file.
 *
 * This function handles parsing the COFF from the source buffer, allocating
 * and mapping the sections, resolving external symbols using the provided callback,
 * applying relocations, and finally applying memory protections.
 *
 * @param buf Raw buffer containing the COFF object data.
 * @param mem_api Memory API to use for allocation/protection.
 * @param mod_api Module API to use for resolving external symbols.
 * @param ctx_out Output loader context. Will contain the loaded execution state.
 * @return SND_OK on success, or an error status on failure. If a failure occurs
 * after memory allocation, the memory is automatically freed.
 */
snd_status_t snd_ldr_coff_load(snd_ldr_coff_ctx_t *ctx_out);

/**
 * @brief Executes the loaded COFF image.
 *
 * @param ctx Loader context containing the loaded COFF image.
 * @param entry_name Name of the entry point symbol to execute.
 * @param bof_args Arguments to pass to the entry point.
 * @param bof_arg_len Length of the bof_args buffer.
 * @return SND_OK on success, or an error status on failure.
 */
snd_status_t snd_ldr_coff_execute_image(snd_ldr_coff_ctx_t *ctx, const char *entry_name, char *bof_args,
                                        int bof_arg_len);

SND_END_EXTERN_C

#endif // SND_LOADERS_COFF_CHAIN_H
