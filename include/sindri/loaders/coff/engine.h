#ifndef SND_LOADERS_COFF_ENGINE_H
#define SND_LOADERS_COFF_ENGINE_H

#include <sindri/common/buffer.h>
#include <sindri/common/macros.h>
#include <sindri/internal/windows/types.h>
#include <sindri/parsers/coff/parser.h>
#include <sindri/primitives/os_api.h>
#include <sindri/status/core.h>

SND_BEGIN_EXTERN_C

#define SND_COFF_METADATA_ALIGNMENT      16
#define SND_X64_TRAMPOLINE_SIZE          16
#define SND_X64_TRAMPOLINE_DISP_OFFSET   2
#define SND_X64_TRAMPOLINE_TARGET_OFFSET 6

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

/**
 * @brief Converts a COFF loader stage enum to a human-readable string description.
 * @param stage The stage enum value.
 * @retval Pointer to a null-terminated stage description.
 */
SND_FORCE_INLINE const char *snd_ldr_coff_stage_to_string(snd_ldr_coff_stage_t stage) {
#if SND_DEBUG
    switch (stage) {
    case SND_COFF_STAGE_UNINITIALIZED:
        return "UNINITIALIZED";
    case SND_COFF_STAGE_PARSED:
        return "PARSED";
    case SND_COFF_STAGE_MEM_ALLOCATED:
        return "MEM_ALLOCATED";
    case SND_COFF_STAGE_SECTIONS_MAPPED:
        return "SECTIONS_MAPPED";
    case SND_COFF_STAGE_SYMBOLS_RESOLVED:
        return "SYMBOLS_RESOLVED";
    case SND_COFF_STAGE_RELOCATED:
        return "RELOCATED";
    case SND_COFF_STAGE_READY_FOR_EXECUTION:
        return "READY_FOR_EXECUTION";
    case SND_COFF_STAGE_EXECUTED:
        return "EXECUTED";
    default:
        return "UNKNOWN_CORRUPTED";
    }
#else
    (void)stage;
    return "";
#endif
}

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
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the memory API, or its
 * `alloc` callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the loader stage is invalid.
 * @retval SND_STATUS_COFF_LOADER_MAP_SIZE_OVERFLOW If the mapping size
 * overflows.
 * @retval SND_STATUS_COFF_LOADER_BSS_SIZE_OVERFLOW If BSS size arithmetic
 * overflows.
 * @retval SND_STATUS_COFF_LOADER_TRAMPOLINE_OVERFLOW If trampoline allocation
 * arithmetic overflows.
 * @retval SND_STATUS_COFF_LOADER_VIRTUAL_SIZE_OVERFLOW If virtual size
 * arithmetic overflows.
 * @retval SND_STATUS_SYMBOL_ENTRY_OUT_OF_RANGE If an auxiliary symbol record
 * exceeds the symbol table.
 * @retval Any error returned by `snd_coff_decode_symbol` or
 * `snd_memory_api_t::alloc`.
 */
snd_status_t snd_ldr_coff_allocate_and_copy_sections(snd_ldr_coff_ctx_t *ctx);

/**
 * @brief Resolves external symbols using the provided callback.
 * @param ctx The loader context.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the module API, or one of its
 * required callbacks is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the loader stage is invalid.
 * @retval SND_STATUS_COFF_LOADER_SYMBOL_MISSING If a symbol table entry is
 * missing.
 * @retval SND_STATUS_SYMBOL_ENTRY_OUT_OF_RANGE If an auxiliary symbol record
 * exceeds the symbol table.
 * @retval Any error returned by `snd_coff_decode_symbol`,
 * `snd_module_api_t::load_library`, or
 * `snd_module_api_t::get_proc_address`.
 */
snd_status_t snd_ldr_coff_resolve_symbols(snd_ldr_coff_ctx_t *ctx);

/**
 * @brief Applies relocations (x86 and x64) to the mapped sections.
 * @param ctx The loader context.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the section map, or the symbol
 * map is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the loader stage is invalid.
 * @retval SND_STATUS_COFF_LOADER_SYMBOL_MISSING If a relocation references a
 * missing symbol.
 * @retval SND_STATUS_SYMBOL_ADDRESS_NULL If a relocation symbol has no
 * resolved address.
 * @retval SND_STATUS_COFF_LOADER_RELOC_OUT_OF_RANGE If a relocation target is
 * outside the mapped section.
 * @retval SND_STATUS_RELOCATION_UNKNOWN_TYPE If the relocation type is unknown.
 * @retval Any error returned by `snd_coff_get_relocations`,
 * `snd_coff_get_relocation_patch_size`, or
 * `snd_memory_api_t::protect`.
 */
snd_status_t snd_ldr_coff_apply_relocations(snd_ldr_coff_ctx_t *ctx);

/**
 * @brief Applies final memory protections based on section characteristics.
 * @param ctx The loader context.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the memory API, or its
 * `protect` callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the loader stage is invalid.
 * @retval Any error returned by `snd_memory_api_t::protect`.
 */
snd_status_t snd_ldr_coff_apply_memory_protections(snd_ldr_coff_ctx_t *ctx);

/**
 * @brief Frees the loaded COFF object and its associated section map.
 * @param ctx The loader context.
 */
void snd_ldr_coff_free_mapped_image(snd_ldr_coff_ctx_t *ctx);

SND_END_EXTERN_C

#endif // SND_LOADERS_COFF_ENGINE_H
