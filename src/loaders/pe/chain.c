#include <sindri/common/debug.h>
#include <sindri/common/macros.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/loaders/pe/chain.h>
#include <sindri/loaders/pe/engine.h>
#include <sindri/loaders/pe/status.h>

typedef BOOL(WINAPI *snd_dll_entry_proc_t)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);
typedef void(WINAPI *snd_export_proc_t)(LPVOID, LPVOID, LPVOID, LPVOID);
typedef void (*snd_exe_entry_proc_t)(void);

void snd_ldr_pe_detach_image(snd_ldr_pe_ctx_t *ctx) {
    if (ctx == NULL || ctx->stage != SND_STAGE_EXECUTED) {
        return;
    }

    if (ctx->target.local_base != ctx->target.execution_base) {
        SND_DEBUG_PRINT("[-] Cannot detach: Image was prepared for a remote process.\n");
        return;
    }

    if (ctx->pe.is_dll) {
        if (ctx->target.entry_point != NULL) {
            snd_dll_entry_proc_t dll_main = (snd_dll_entry_proc_t)ctx->target.entry_point;
            dll_main((HINSTANCE)ctx->target.execution_base, SND_DLL_PROCESS_DETACH, NULL);
        }
        snd_ldr_pe_execute_tls_callbacks(ctx, SND_DLL_PROCESS_DETACH);
    }

    snd_ldr_pe_free_mapped_image(ctx);
}

snd_status_t snd_ldr_pe_prepare_image(snd_ldr_pe_ctx_t *ctx) {
    SND_CHECK_NULL(ctx);

    SND_DEBUG_PRINT("[*] Initiating PE reflection chain...\n");

    SND_TRY(snd_pe_parse(ctx->raw_source, FALSE, &ctx->pe));

    ctx->stage = SND_STAGE_PARSED;

    if (!SND_IS_ARCH_COMPATIBLE(ctx->pe.is_64bit)) {
        return SND_ERR(SND_STATUS_ARCH_MISMATCH);
    }

    SND_TRY(snd_ldr_pe_allocate_and_copy_image(ctx));

    snd_status_t status = snd_ldr_pe_apply_relocations(ctx);
    if (SND_FAILED(status)) {
        goto err_cleanup;
    }

    status = snd_ldr_pe_resolve_imports(ctx);
    if (SND_FAILED(status)) {
        goto err_cleanup;
    }

    status = snd_ldr_pe_apply_memory_protections(ctx);
    if (SND_FAILED(status)) {
        goto err_cleanup;
    }

    SND_DEBUG_PRINT("[+] Image successfully prepared and ready for execution.\n");
    return SND_OK;

err_cleanup:
    snd_ldr_pe_free_mapped_image(ctx);
    return status;
}

snd_status_t snd_ldr_pe_execute_image(snd_ldr_pe_ctx_t *ctx) {
    SND_CHECK_NULL(ctx);

    if (ctx->stage < SND_STAGE_READY_FOR_EXECUTION) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Sequence error: Expected stage %s, got %s.",
                           snd_ldr_pe_stage_to_string(SND_STAGE_READY_FOR_EXECUTION),
                           snd_ldr_pe_stage_to_string(ctx->stage));
    }

    if (ctx->target.local_base != ctx->target.execution_base) {
        return SND_ERR_CTX(SND_STATUS_LOCAL_EXECUTION_BLOCKED,
                           "Cannot execute image locally. Image is configured for remote base: 0x%p",
                           ctx->target.execution_base);
    }

    snd_ldr_pe_execute_tls_callbacks(ctx, SND_DLL_PROCESS_ATTACH);

    void *entry_point = ctx->target.entry_point;
    if (ctx->pe.is_dll) {
        if (entry_point == NULL) {
            ctx->stage = SND_STAGE_EXECUTED;
            return SND_OK;
        }

        SND_DEBUG_PRINT("[!] Executing DllMain at 0x%p...\n", entry_point);
        snd_dll_entry_proc_t dll_main = (snd_dll_entry_proc_t)entry_point;
        if (!dll_main((HINSTANCE)ctx->target.execution_base, SND_DLL_PROCESS_ATTACH, NULL)) {
            SND_DEBUG_PRINT("[-] DllMain returned FALSE.\n");
            return SND_ERR(SND_STATUS_DLL_INITIALIZATION_FAILED);
        }

        ctx->stage = SND_STAGE_EXECUTED;
        SND_DEBUG_PRINT("[+] DLL initialized successfully.\n");
        return SND_OK;
    }

    if (entry_point == NULL) {
        return SND_ERR(SND_STATUS_IMAGE_ENTRY_POINT_MISSING);
    }

    SND_DEBUG_PRINT("[!] Jumping to EXE Entry Point at 0x%p...\n", entry_point);
    snd_exe_entry_proc_t exe_entry = (snd_exe_entry_proc_t)entry_point;
    exe_entry();
    ctx->stage = SND_STAGE_EXECUTED;
    return SND_OK;
}
