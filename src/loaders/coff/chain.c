#include <sindri/common/debug.h>
#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <sindri/loaders/coff/chain.h>
#include <sindri/loaders/coff/status.h>
#include <sindri/status.h>
#include <sindri/status/core.h>

typedef void (*snd_coff_entry_t)(char *args, int arg_len);

snd_status_t snd_ldr_coff_load(snd_ldr_coff_ctx_t *ctx) {
    SND_CHECK_NULL(ctx);

    snd_status_t status;

    SND_DEBUG_PRINT("[*] Initiating COFF reflection chain...\n");

    SND_TRY(snd_coff_parse(ctx->raw_source, &ctx->coff));
    ctx->stage = SND_COFF_STAGE_PARSED;

    if (!SND_IS_ARCH_COMPATIBLE(ctx->coff.is_64bit)) {
        return SND_ERR(SND_STATUS_ARCH_MISMATCH);
    }

    status = snd_ldr_coff_allocate_and_copy_sections(ctx);
    if (SND_FAILED(status)) {
        goto failure;
    }

    status = snd_ldr_coff_resolve_symbols(ctx);
    if (SND_FAILED(status)) {
        goto failure;
    }

    status = snd_ldr_coff_apply_relocations(ctx);
    if (SND_FAILED(status)) {
        goto failure;
    }

    status = snd_ldr_coff_apply_memory_protections(ctx);
    if (SND_FAILED(status)) {
        goto failure;
    }

    SND_DEBUG_PRINT("[+] Image successfully prepared and ready for execution.\n");
    return SND_OK;

failure:
    if (ctx->stage >= SND_COFF_STAGE_SECTIONS_MAPPED) {
        snd_ldr_coff_free_mapped_image(ctx);
    }

    return status;
}

snd_status_t snd_ldr_coff_execute_image(snd_ldr_coff_ctx_t *ctx, const char *entry_name, char *bof_args,
                                        int bof_arg_len) {
    SND_CHECK_NULL(ctx);

    if (ctx->stage < SND_COFF_STAGE_READY_FOR_EXECUTION) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Sequence error: Expected stage >= %s, got %s.",
                           snd_ldr_coff_stage_to_string(SND_COFF_STAGE_READY_FOR_EXECUTION),
                           snd_ldr_coff_stage_to_string(ctx->stage));
    }

    if (ctx->target.local_base != ctx->target.execution_base) {
        return SND_ERR_CTX(SND_STATUS_COFF_LOADER_LOCAL_EXECUTION_BLOCKED,
                           "Cannot execute COFF locally. Image is configured for remote base: 0x%p",
                           ctx->target.execution_base);
    }

    if (!SND_IS_ARCH_COMPATIBLE(ctx->coff.is_64bit)) {
        return SND_ERR(SND_STATUS_ARCH_MISMATCH);
    }

    SIZE_T ep_offset = 0;
    SND_TRY(snd_ldr_coff_resolve_entry_offset(ctx, entry_name, &ep_offset));

    snd_coff_entry_t entry_func = (snd_coff_entry_t)SND_PTR_ADD(ctx->target.local_base, ep_offset);
    entry_func(bof_args, bof_arg_len);

    ctx->stage = SND_COFF_STAGE_EXECUTED;
    return SND_OK;
}
