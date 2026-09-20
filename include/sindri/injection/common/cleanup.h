#ifndef SND_INJECTION_COMMON_CLEANUP_H
#define SND_INJECTION_COMMON_CLEANUP_H

#include <sindri/common/macros.h>
#include <sindri/injection/common/context.h>
#include <sindri/status/core.h>

SND_BEGIN_EXTERN_C

/**
 * @def SND_INJ_CLEANUP_EXIT_CODE
 * @brief Exit code used for cleanup termination.
 */
#define SND_INJ_CLEANUP_EXIT_CODE 1U

/**
 * @def SND_INJ_TRY
 * @brief Returns a failed status after best-effort injection cleanup.
 * @param ctx Injection context to clean.
 * @param expr Status-producing expression to evaluate.
 */
#define SND_INJ_TRY(ctx, expr)                                                                                         \
    do {                                                                                                               \
        snd_status_t _snd_inj_status = (expr);                                                                         \
        if (SND_FAILED(_snd_inj_status)) {                                                                             \
            snd_inj_cleanup(ctx);                                                                                      \
            return _snd_inj_status;                                                                                    \
        }                                                                                                              \
    } while (0)

/**
 * @def SND_INJ_TRY_WITH_CLEANUP
 * @brief Returns a failed status after injection and caller cleanup.
 * @param inj_ctx Injection context to clean.
 * @param cleanup_expr Additional cleanup expression, such as loader cleanup.
 * @param expr Status-producing expression to evaluate.
 */
#define SND_INJ_TRY_WITH_CLEANUP(inj_ctx, cleanup_expr, expr)                                                          \
    do {                                                                                                               \
        snd_status_t _snd_inj_status = (expr);                                                                         \
        if (SND_FAILED(_snd_inj_status)) {                                                                             \
            snd_inj_cleanup(inj_ctx);                                                                                  \
            cleanup_expr;                                                                                              \
            return _snd_inj_status;                                                                                    \
        }                                                                                                              \
    } while (0)

/**
 * @brief Best-effort cleanup of injection-owned handles and pre-execution memory.
 *
 * Cleanup never replaces the operation status. Missing backend capabilities or
 * cleanup failures are ignored. Remote memory is released only before execution
 * and only when the process backend supplies `free_remote`.
 *
 * @param ctx Context to clean; NULL is accepted.
 */
void snd_inj_cleanup(snd_inj_ctx_t *ctx);

SND_END_EXTERN_C

#endif // SND_INJECTION_COMMON_CLEANUP_H
