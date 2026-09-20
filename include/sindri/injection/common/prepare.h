#ifndef SND_INJECTION_COMMON_PREPARE_H
#define SND_INJECTION_COMMON_PREPARE_H

#include <sindri/common/macros.h>
#include <sindri/injection/common/context.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

typedef struct _snd_ldr_pe_ctx   snd_ldr_pe_ctx_t;
typedef struct _snd_ldr_coff_ctx snd_ldr_coff_ctx_t;

/** Selects whether preparation opens an existing process or creates one. */
typedef enum {
    SND_INJ_TARGET_EXISTING,
    SND_INJ_TARGET_SUSPENDED,
} snd_inj_target_mode_t;

/**
 * @brief Prepares and stages a PE for injection.
 * @param ldr_ctx PE loader context with source and loader APIs initialized.
 * @param inj_ctx Injection context with target and process APIs initialized.
 * @param target_mode Target acquisition mode.
 * @retval SND_OK When the image is staged and ready for technique execution.
 * @retval Any error returned by PE parsing, loading, target, or staging steps.
 */
snd_status_t snd_inj_prepare_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx, snd_inj_target_mode_t target_mode);

/**
 * @brief Prepares and stages a COFF/BOF for injection.
 * @param ldr_ctx COFF loader context with source and loader APIs initialized.
 * @param inj_ctx Injection context with target and process APIs initialized.
 * @param target_mode Target acquisition mode.
 * @param entry_point BOF entry symbol; NULL selects `go`.
 * @param args Optional BOF argument buffer.
 * @param arg_len Argument buffer length; must not be negative.
 * @retval SND_OK When the image is staged and ready for technique execution.
 * @retval SND_STATUS_INVALID_PARAMETERS_COMBINATION For invalid arguments.
 * @retval Any error returned by COFF parsing, loading, target, or staging steps.
 */
snd_status_t snd_inj_prepare_coff(snd_ldr_coff_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx,
                                  snd_inj_target_mode_t target_mode, const char *entry_point, void *args, int arg_len);

SND_END_EXTERN_C

#endif // SND_INJECTION_COMMON_PREPARE_H
