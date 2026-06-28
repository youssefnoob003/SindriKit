#include "sindri/primitives/memory.h"

#include <sindri.h>

int main() {
    // Your exe path here
    const char *file_path = "payload.exe";

    snd_status_t status;

    snd_buffer_t     file_buf = {0};
    snd_ldr_pe_ctx_t ctx      = {0};

    PVOID ntdll;
    status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (SND_FAILED(status)) {
        goto cleanup;
    }
    snd_syscall_set_ntdll(ntdll);

    /* Configure the cascading syscall resolution strategy. */
    snd_syscall_strategy_set(snd_syscall_resolve_ssn_scan);

    ctx.mem_api = &snd_mem_win;
    ctx.mod_api = &snd_mod_nt;

    status = snd_disk_buffer_load(file_path, &file_buf);
    if (SND_FAILED(status)) {
        goto cleanup;
    }

    ctx.raw_source = &file_buf;

    status = snd_ldr_pe_prepare_image(&ctx);
    if (SND_FAILED(status)) {
        goto cleanup;
    }

    status = snd_ldr_pe_execute_image(&ctx);
    if (SND_FAILED(status)) {
        goto cleanup;
    }

cleanup:
    snd_ldr_pe_detach_image(&ctx);
    snd_ldr_pe_free_mapped_image(&ctx);
    snd_buffer_free(&file_buf);

    if (SND_FAILED(status)) {
        return status.code;
    }

    return status.code;
}
