#include <sindri/common/debug.h>
#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/loaders/coff/chain.h>
#include <sindri/parsers/coff/symbols.h>

typedef void (*snd_coff_entry_t)(char *args, int arg_len);

snd_status_t snd_ldr_coff_load(snd_ldr_coff_ctx_t *ctx) {
    if (ctx == NULL) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    snd_status_t status;

    SND_DEBUG_PRINT("[*] Initiating COFF reflection chain...\n");

    status = snd_coff_parse(ctx->raw_source, &ctx->coff);
    if (SND_FAILED(status)) {
        return status;
    }
    ctx->stage = SND_COFF_STAGE_PARSED;

    status = snd_ldr_coff_allocate_and_copy_sections(ctx);
    if (SND_FAILED(status))
        goto failure;

    status = snd_ldr_coff_resolve_symbols(ctx);
    if (SND_FAILED(status))
        goto failure;

    status = snd_ldr_coff_apply_relocations(ctx);
    if (SND_FAILED(status))
        goto failure;

    status = snd_ldr_coff_apply_memory_protections(ctx);
    if (SND_FAILED(status))
        goto failure;

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
    if (ctx == NULL) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    PIMAGE_SYMBOL entry_sym = NULL;
    snd_status_t  status    = snd_coff_find_symbol_by_name(&ctx->coff, entry_name, &entry_sym, NULL);

    if (SND_FAILED(status)) {
        return status;
    }

    if (entry_sym->SectionNumber <= 0 || (DWORD)entry_sym->SectionNumber > ctx->coff.sections_count) {
        return SND_ERR(SND_STATUS_COFF_ENTRY_NOT_RWX);
    }

    LPVOID section_base = ctx->target.section_map[entry_sym->SectionNumber];
    if (!section_base) {
        return SND_ERR(SND_STATUS_COFF_ENTRY_UNMAPPED);
    }

    snd_coff_entry_t entry_func = (snd_coff_entry_t)((ULONG_PTR)section_base + entry_sym->Value);
    entry_func(bof_args, bof_arg_len);

    return status;
}
