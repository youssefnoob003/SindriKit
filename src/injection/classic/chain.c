#include <sindri/injection/classic/chain.h>
#include <sindri/injection/common.h>
#include <sindri/injection/common/prepare.h>
#include <sindri/loaders/coff/engine.h>
#include <sindri/loaders/pe/engine.h>

snd_status_t snd_inj_classic_shell(snd_inj_ctx_t *ctx) {
    SND_CHECK_NULL(ctx);
    SND_INJ_TRY(ctx, snd_inj_open_target(ctx));
    SND_INJ_TRY(ctx, snd_inj_alloc_remote(ctx));
    SND_INJ_TRY(ctx, snd_inj_write_payload(ctx));
    SND_INJ_TRY(ctx, snd_inj_set_protections(ctx));
    SND_INJ_TRY(ctx, snd_inj_classic_execute(ctx));
    return SND_OK;
}

snd_status_t snd_inj_classic_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx) {
    SND_INJ_TRY(inj_ctx, snd_inj_prepare_pe(ldr_ctx, inj_ctx, SND_INJ_TARGET_EXISTING));
    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_pe_free_mapped_image(ldr_ctx), snd_inj_classic_execute(inj_ctx));
    return SND_OK;
}

snd_status_t snd_inj_classic_coff(snd_ldr_coff_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx, const char *entry_point,
                                  void *args, int arg_len) {
    SND_INJ_TRY(inj_ctx, snd_inj_prepare_coff(ldr_ctx, inj_ctx, SND_INJ_TARGET_EXISTING, entry_point, args, arg_len));
    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_coff_free_mapped_image(ldr_ctx), snd_inj_classic_execute(inj_ctx));
    return SND_OK;
}
