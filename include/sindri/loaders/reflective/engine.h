#ifndef SND_LOADERS_REFLECTIVE_ENGINE_H
#define SND_LOADERS_REFLECTIVE_ENGINE_H

#include <sindri/common/buffer.h>
#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/primitives/os_api.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

typedef struct {
    LPVOID   local_base;
    LPVOID   execution_base;
    LONG_PTR delta_offset;
    LPVOID   entry_point;
    SIZE_T   allocated_size;
} snd_pe_target_t;

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

typedef struct _snd_ldr_pe_ctx {
    const snd_buffer_t *raw_source;
    snd_pe_parser_t     pe;
    snd_pe_target_t     target;
    snd_ldr_pe_stage_t  stage;

    const snd_memory_api_t *mem_api;
    const snd_module_api_t *mod_api;

} snd_ldr_pe_ctx_t;

/**
 * @brief Portable macro to resolve and call a reflectively loaded DLL export
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
 * @brief Portable macro to resolve and call a reflectively loaded DLL export
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

/**
 * @brief Validates payload architecture.
 * @param ctx The loader context with parsed PE.
 * @return SND_OK on match, otherwise SND_STATUS_ARCH_MISMATCH.
 */
snd_status_t snd_ldr_pe_compatibility_check(snd_ldr_pe_ctx_t *ctx);

/**
 * @brief Allocates memory and copies sections.
 * @param ctx The loader context. ctx->virtual_base will be populated on
 * success.
 * @return SND_OK on success, otherwise an allocation error.
 */
snd_status_t snd_ldr_pe_allocate_and_copy_image(snd_ldr_pe_ctx_t *ctx);

/**
 * @brief Applies base relocation fixups.
 * @param ctx The loader context containing the mapped virtual base.
 * @return SND_OK on success, otherwise a relocation error.
 */
snd_status_t snd_ldr_pe_apply_relocations(snd_ldr_pe_ctx_t *ctx);

/**
 * @brief Resolves imports and patches IAT.
 * @param ctx The loader context.
 * @return SND_OK on success, otherwise a resolution error.
 */
snd_status_t snd_ldr_pe_resolve_imports(snd_ldr_pe_ctx_t *ctx);

/**
 * @brief Applies final section page protections.
 * @param ctx The loader context.
 * @return SND_OK on success, otherwise a protection error.
 */
snd_status_t snd_ldr_pe_apply_memory_protections(snd_ldr_pe_ctx_t *ctx);

/**
 * @brief Executes TLS callbacks from a loaded image.
 * @param ctx The loader context.
 * @param reason The reason for the call (e.g., DLL_PROCESS_ATTACH).
 */
void snd_ldr_pe_execute_tls_callbacks(snd_ldr_pe_ctx_t *ctx, DWORD reason);

/**
 * @brief Resolves the entry point pointer.
 * @param ctx The loader context.
 * @return SND_OK on success, error code on failure.
 */
snd_status_t snd_ldr_pe_get_entry_point(snd_ldr_pe_ctx_t *ctx);

/**
 * @brief Resolves an exported symbol address.
 * @param ctx The loader context.
 * @param func_name Export name to resolve.
 * @param func_addr_out Receives resolved export pointer.
 * @return SND_OK on success, error code on failure.
 */
snd_status_t snd_ldr_pe_get_proc_address(snd_ldr_pe_ctx_t *ctx, const char *func_name, FARPROC *func_addr_out);

/**
 * @brief Frees memory associated with the mapped reflective image.
 * @param ctx The loader context.
 */
void snd_ldr_pe_free_mapped_image(snd_ldr_pe_ctx_t *ctx);

SND_END_EXTERN_C

#endif // SND_LOADERS_REFLECTIVE_ENGINE_H
