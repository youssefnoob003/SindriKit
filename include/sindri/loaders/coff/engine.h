#ifndef SND_LOADERS_COFF_ENGINE_H
#define SND_LOADERS_COFF_ENGINE_H

#include <sindri/common/buffer.h>
#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/parsers/coff/parser.h>
#include <sindri/primitives/os_api.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

SND_SHUFFLE_START
typedef struct {
    LPVOID local_base;
    LPVOID execution_base;
    SIZE_T allocated_size;

    LPVOID *section_map;
    LPVOID *symbol_map;

    LPVOID iat_base;
    LPVOID trampolines_base;
    LPVOID bss_base;
} snd_coff_target_t;
SND_SHUFFLE_END

typedef enum {
    SND_COFF_STAGE_UNINITIALIZED = 0,
    SND_COFF_STAGE_PARSED,
    SND_COFF_STAGE_MEM_ALLOCATED,
    SND_COFF_STAGE_SECTIONS_MAPPED,
    SND_COFF_STAGE_SYMBOLS_RESOLVED,
    SND_COFF_STAGE_RELOCATED,
    SND_COFF_STAGE_READY_FOR_EXECUTION,
    SND_COFF_STAGE_EXECUTED,
} snd_ldr_coff_stage_t;

SND_SHUFFLE_START
typedef struct _snd_ldr_coff_ctx {
    const snd_buffer_t  *raw_source;
    snd_coff_parser_t    coff;
    snd_coff_target_t    target;
    snd_ldr_coff_stage_t stage;

    const snd_memory_api_t *mem_api;
    const snd_module_api_t *mod_api;
} snd_ldr_coff_ctx_t;
SND_SHUFFLE_END

/**
 * @brief Allocates memory and copies sections from the parsed COFF.
 * @param ctx The loader context.
 * @return SND_OK on success.
 */
snd_status_t snd_ldr_coff_allocate_and_copy_sections(snd_ldr_coff_ctx_t *ctx);

/**
 * @brief Resolves external symbols using the provided callback.
 * @param ctx The loader context.
 * @return SND_OK on success.
 */
snd_status_t snd_ldr_coff_resolve_symbols(snd_ldr_coff_ctx_t *ctx);

/**
 * @brief Applies relocations (x86 and x64) to the mapped sections.
 * @param ctx The loader context.
 * @return SND_OK on success.
 */
snd_status_t snd_ldr_coff_apply_relocations(snd_ldr_coff_ctx_t *ctx);

/**
 * @brief Applies final memory protections based on section characteristics.
 * @param ctx The loader context.
 * @return SND_OK on success.
 */
snd_status_t snd_ldr_coff_apply_memory_protections(snd_ldr_coff_ctx_t *ctx);

/**
 * @brief Frees the loaded COFF object and its associated section map.
 * @param ctx The loader context.
 */
void snd_ldr_coff_free_mapped_image(snd_ldr_coff_ctx_t *ctx);

SND_END_EXTERN_C

#endif // SND_LOADERS_COFF_ENGINE_H
