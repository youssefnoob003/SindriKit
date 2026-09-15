#ifndef SND_LOADERS_REFLECTIVE_ENGINE_H
#define SND_LOADERS_REFLECTIVE_ENGINE_H

#include <sindri/common/buffer.h>
#include <sindri/common/macros.h>
#include <sindri/internal/windows/types.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/primitives/os_api.h>
#include <sindri/status/core.h>

SND_BEGIN_EXTERN_C

SND_SHUFFLE_START
typedef struct {
    LPVOID   local_base;
    LPVOID   execution_base;
    SIZE_T   allocated_size;
    LONG_PTR delta_offset;
    LPVOID   entry_point;
} snd_pe_target_t;
SND_SHUFFLE_END

typedef enum {
    SND_STAGE_UNINITIALIZED = 0,
    SND_STAGE_PARSED,
    SND_STAGE_MEM_ALLOCATED,
    SND_STAGE_SECTIONS_MAPPED,
    SND_STAGE_RELOCATED,
    SND_STAGE_IMPORTS_RESOLVED,
    SND_STAGE_READY_FOR_EXECUTION,
    SND_STAGE_EXECUTED,
} snd_ldr_pe_stage_t;

SND_SHUFFLE_START
typedef struct _snd_ldr_pe_ctx {
    const snd_buffer_t *raw_source;
    snd_pe_parser_t     pe;
    snd_pe_target_t     target;
    snd_ldr_pe_stage_t  stage;

    const snd_memory_api_t *mem_api;
    const snd_module_api_t *mod_api;

} snd_ldr_pe_ctx_t;
SND_SHUFFLE_END

/**
 * @brief Portable macro to resolve and call a pely loaded DLL export
 * with arbitrary arguments.
 * @param ctx         Pointer to the initialized loader context.
 * @param name        Plaintext string name of the target export function.
 * @param signature   The function pointer type signature to cast the export to.
 * @param status_out  A variable of type 'snd_status_t' that will receive the
 * status result.
 * @param ...         The comma-separated arguments to pass directly to the
 * target function.
 */
#define SND_CALL_EXPORT(ctx, name, signature, status_out, ...)                                                         \
    do {                                                                                                               \
        FARPROC _proc = NULL;                                                                                          \
        (status_out)  = snd_ldr_pe_get_proc_address((ctx), (name), &_proc);                                            \
        if ((status_out).code == SND_SUCCESS && _proc != NULL) {                                                       \
            ((signature)_proc)(__VA_ARGS__);                                                                           \
        }                                                                                                              \
    } while (0)

/**
 * @brief Portable macro to resolve and call a pely loaded DLL export
 * and capture its return value.
 * @param ctx         Pointer to the initialized loader context.
 * @param name        Plaintext string name of the target export function.
 * @param signature   The function pointer type signature to cast the export to.
 * @param status_out  A variable of type 'snd_status_t' that will receive the
 * status result.
 * @param ret_out     Variable to store the return value of the export.
 * @param ...         The comma-separated arguments to pass directly to the
 * target function.
 */
#define SND_CALL_EXPORT_RET(ctx, name, signature, status_out, ret_out, ...)                                            \
    do {                                                                                                               \
        FARPROC _proc = NULL;                                                                                          \
        (status_out)  = snd_ldr_pe_get_proc_address((ctx), (name), &_proc);                                            \
        if ((status_out).code == SND_SUCCESS && _proc != NULL) {                                                       \
            (ret_out) = ((signature)_proc)(__VA_ARGS__);                                                               \
        }                                                                                                              \
    } while (0)

SND_FORCE_INLINE const char *snd_ldr_pe_stage_to_string(snd_ldr_pe_stage_t stage) {
#if SND_DEBUG
    switch (stage) {
    case SND_STAGE_UNINITIALIZED:
        return "UNINITIALIZED";
    case SND_STAGE_PARSED:
        return "PARSED";
    case SND_STAGE_MEM_ALLOCATED:
        return "MEM_ALLOCATED";
    case SND_STAGE_SECTIONS_MAPPED:
        return "SECTIONS_MAPPED";
    case SND_STAGE_RELOCATED:
        return "RELOCATED";
    case SND_STAGE_IMPORTS_RESOLVED:
        return "IMPORTS_RESOLVED";
    case SND_STAGE_READY_FOR_EXECUTION:
        return "READY_FOR_EXECUTION";
    case SND_STAGE_EXECUTED:
        return "EXECUTED";
    default:
        return "UNKNOWN_CORRUPTED";
    }
#else
    (void)stage;
    return "";
#endif
}

/**
 * @brief Allocates memory and copies sections.
 * @param ctx The loader context. ctx->virtual_base will be populated on
 * success.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the memory API, or its
 * `alloc` callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the loader stage is invalid.
 * @retval SND_STATUS_HEADERS_SIZE_INVALID If image headers exceed the source
 * or target allocation bounds.
 * @retval SND_STATUS_SECTION_TABLE_MISSING If the section table is absent.
 * @retval SND_STATUS_SECTION_SIZE_INVALID If a section exceeds the target
 * allocation.
 * @retval SND_STATUS_CORRUPTED_STAGE If the parsed source or image size is
 * invalid.
 * @retval SND_STATUS_IMAGE_SIZE_NULL If `SizeOfImage` is zero.
 * @retval Any error returned by `snd_memory_api_t::alloc` or
 * `snd_pe_parse` while mapping the image.
 */
snd_status_t snd_ldr_pe_allocate_and_copy_image(snd_ldr_pe_ctx_t *ctx);

/**
 * @brief Applies base relocation fixups.
 * @param ctx The loader context containing the mapped virtual base.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the loader stage is invalid.
 * @retval SND_STATUS_CORRUPTED_STAGE If the mapped image state is invalid.
 * @retval SND_STATUS_RELOCATION_DIRECTORY_MISSING If relocations are required
 * but the relocation directory is absent.
 * @retval SND_STATUS_RELOCATION_DIRECTORY_STRIPPED If relocations are stripped.
 * @retval SND_STATUS_RELOCATION_TYPE_INVALID If a relocation type is unsupported.
 * @retval Any error returned by `snd_pe_get_directory`,
 * `snd_pe_get_reloc_block`, or `snd_pe_get_reloc_entry`.
 */
snd_status_t snd_ldr_pe_apply_relocations(snd_ldr_pe_ctx_t *ctx);

/**
 * @brief Resolves imports and patches IAT.
 * @param ctx The loader context.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the module API, or one of its
 * required callbacks is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the loader stage is invalid.
 * @retval SND_STATUS_CORRUPTED_STAGE If the mapped image state is invalid.
 * @retval Any error returned by `snd_pe_get_import_descriptor`,
 * `snd_pe_get_import_name`, `snd_pe_get_import_thunk`,
 * `snd_module_api_t::load_library`, or
 * `snd_module_api_t::get_proc_address`.
 */
snd_status_t snd_ldr_pe_resolve_imports(snd_ldr_pe_ctx_t *ctx);

/**
 * @brief Applies final section page protections.
 * @param ctx The loader context.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the memory API, or its
 * `protect` callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the loader stage is invalid.
 * @retval SND_STATUS_CORRUPTED_STAGE If the mapped image state is invalid.
 * @retval Any error returned by `snd_memory_api_t::protect`.
 */
snd_status_t snd_ldr_pe_apply_memory_protections(snd_ldr_pe_ctx_t *ctx);

/**
 * @brief Executes TLS callbacks from a loaded image.
 * @param ctx The loader context.
 * @param reason The reason for the call (e.g., DLL_PROCESS_ATTACH).
 */
void snd_ldr_pe_execute_tls_callbacks(snd_ldr_pe_ctx_t *ctx, DWORD reason);

/**
 * @brief Resolves an exported symbol address.
 * @param ctx The loader context.
 * @param func_name Export name to resolve.
 * @param func_addr_out Receives resolved export pointer.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, @p func_name, or
 * @p func_addr_out is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the image is not ready for execution.
 * @retval Any error returned by `snd_pe_get_export_address`.
 */
snd_status_t snd_ldr_pe_get_proc_address(const snd_ldr_pe_ctx_t *ctx, const char *func_name, FARPROC *func_addr_out);

/**
 * @brief Frees memory associated with the mapped pe image.
 * @param ctx The loader context.
 */
void snd_ldr_pe_free_mapped_image(snd_ldr_pe_ctx_t *ctx);

SND_END_EXTERN_C

#endif // SND_LOADERS_REFLECTIVE_ENGINE_H
