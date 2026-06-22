#include <sindri/common.h>
#include <sindri/common/status.h>
#include <sindri/loaders/reflective/chain.h>
#include <windows.h>

typedef BOOL(WINAPI *snd_dll_entry_proc_t)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);
typedef void(WINAPI *snd_export_proc_t)(LPVOID, LPVOID, LPVOID, LPVOID);
typedef void (*snd_exe_entry_proc_t)(void);

void snd_detach_reflective_image(snd_loader_ctx_t *ctx) {
    if (ctx == NULL || ctx->stage != SND_STAGE_EXECUTED) {
        return;
    }

    if (ctx->pe.is_dll) {
        snd_get_entry_point(ctx);
        if (ctx->target.entry_point != NULL) {
            snd_dll_entry_proc_t dll_main = (snd_dll_entry_proc_t)ctx->target.entry_point;
            dll_main((HINSTANCE)ctx->target.virtual_base, DLL_PROCESS_DETACH, NULL);
        }
        snd_execute_tls_callbacks(ctx, DLL_PROCESS_DETACH);
    }

    snd_free_mapped_image(ctx);
}

snd_status_t snd_prepare_reflective_image(snd_loader_ctx_t *ctx) {
    if (ctx == NULL) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }

    snd_status_t status;

    SND_DEBUG_PRINT("[*] Initiating PE reflection chain...\n");

    status = snd_pe_parse(ctx->raw_source, FALSE, &ctx->pe);
    if (status.code != SND_SUCCESS) {
        return status;
    }

    ctx->stage = SND_STAGE_PARSED;

    status = snd_compatibility_check(ctx);
    if (status.code != SND_SUCCESS) {
        return status;
    }

    status = snd_allocate_and_copy_image(ctx);
    if (status.code != SND_SUCCESS) {
        return status;
    }
    status = snd_apply_relocations(ctx);
    if (status.code != SND_SUCCESS) {
        return status;
    }
    status = snd_resolve_imports(ctx);
    if (status.code != SND_SUCCESS) {
        return status;
    }
    status = snd_apply_memory_protections(ctx);
    if (status.code != SND_SUCCESS) {
        return status;
    }

    SND_DEBUG_PRINT("[+] Image successfully prepared and ready for execution.\n");
    return SND_OK;
}

snd_status_t snd_execute_reflective_image(snd_loader_ctx_t *ctx) {
    if (ctx == NULL) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }

    if (ctx->stage < SND_STAGE_READY_FOR_EXECUTION) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE_SEQUENCE,
                           "Cannot execute image. Current stage is %d, required "
                           "is READY_FOR_EXECUTION.",
                           ctx->stage);
    }

    snd_status_t status = snd_get_entry_point(ctx);
    if (status.code != SND_SUCCESS) {
        return status;
    }

    snd_execute_tls_callbacks(ctx, DLL_PROCESS_ATTACH);

    void *entry_point = ctx->target.entry_point;
    if (ctx->pe.is_dll) {
        if (entry_point == NULL) {
            ctx->stage = SND_STAGE_EXECUTED;
            return SND_OK;
        }

        SND_DEBUG_PRINT("[!] Executing DllMain at 0x%p...\n", entry_point);
        snd_dll_entry_proc_t dll_main = (snd_dll_entry_proc_t)entry_point;
        if (!dll_main((HINSTANCE)ctx->target.virtual_base, DLL_PROCESS_ATTACH, NULL)) {
            SND_DEBUG_PRINT("[-] DllMain returned FALSE.\n");
            return SND_ERR(SND_STATUS_DLL_INITIALIZATION_FAILED);
        }

        ctx->stage = SND_STAGE_EXECUTED;
        SND_DEBUG_PRINT("[+] DLL initialized successfully.\n");
        return SND_OK;
    }

    if (entry_point == NULL) {
        return SND_ERR(SND_STATUS_ENTRY_POINT_NOT_FOUND);
    }

    SND_DEBUG_PRINT("[!] Jumping to EXE Entry Point at 0x%p...\n", entry_point);
    snd_exe_entry_proc_t exe_entry = (snd_exe_entry_proc_t)entry_point;
    exe_entry();
    ctx->stage = SND_STAGE_EXECUTED;
    return SND_OK;
}
