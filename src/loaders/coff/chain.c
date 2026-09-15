#include <sindri/common/debug.h>
#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <sindri/common/string.h>
#include <sindri/loaders/coff/chain.h>
#include <sindri/parsers/coff/symbols.h>
#include <sindri/status.h>
#include <sindri/status/core.h>

typedef void (*snd_coff_entry_t)(char *args, int arg_len);

snd_status_t snd_ldr_coff_load(snd_ldr_coff_ctx_t *ctx) {
    SND_CHECK_NULL(ctx);

    snd_status_t status;

    SND_DEBUG_PRINT("[*] Initiating COFF reflection chain...\n");

    SND_TRY(snd_coff_parse(ctx->raw_source, &ctx->coff));
    ctx->stage = SND_COFF_STAGE_PARSED;

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
    if (!SND_IS_ARCH_COMPATIBLE(ctx->coff.is_64bit)) {
        return SND_ERR(SND_STATUS_ARCH_MISMATCH);
    }

    const char *s                                = entry_name ? entry_name : "go";
    char        ep_name[SND_COFF_MAX_SYMBOL_LEN] = {0};

    size_t s_len = snd_strnlen(entry_name, SND_COFF_MAX_SYMBOL_LEN);
    snd_strncpy(ep_name, SND_COFF_MAX_SYMBOL_LEN, s, s_len);

    PSND_IMAGE_SYMBOL entry_sym = NULL;
    SND_TRY(snd_coff_find_symbol_by_name(&ctx->coff, entry_name, s_len, &entry_sym, NULL));
    LPVOID section_base = ctx->target.section_map[entry_sym->SectionNumber];

    snd_coff_entry_t entry_func = (snd_coff_entry_t)(SND_PTR_ADD(section_base, entry_sym->Value));
    entry_func(bof_args, bof_arg_len);

    return SND_OK;
}
