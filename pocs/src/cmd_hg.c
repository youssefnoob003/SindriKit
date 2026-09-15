#include <sindri.h>
#include <unified/commands.h>
#include <unified/common.h>
#include <unified/print.h>

static void print_usage(const char *prog) {
    poc_fprintf("Usage: %s hg\n\n", prog);
    poc_fprintf("Heaven's Gate: Execute 64-bit shellcode from a 32-bit WOW64 process.\n");
    poc_fprintf("No options required; uses built-in test shellcode.\n");
}

int cmd_hg(int argc, char *argv[], const char *prog) {
    for (int i = 0; i < argc; i++) {
        if (poc_strcmp(argv[i], "-h") == 0 || poc_strcmp(argv[i], "--help") == 0) {
            print_usage(prog);
            return SND_SUCCESS;
        }
    }

    log_info("SindriKit Heaven's Gate PoC");

    if (!snd_is_wow64()) {
        log_err("Not running in WOW64. Heaven's Gate requires WOW64.");
        return SND_STATUS_ARCH_MISMATCH;
    }

    log_ok("WOW64 environment detected.");

    unsigned char shellcode64[] = {0x48, 0xB8, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0xC3};

    PVOID pExec = NULL;
#if defined(SND_CRTLESS)
    const snd_memory_api_t *memory = &snd_mem_nt;
#else
    const snd_memory_api_t *memory = &snd_mem_win;
#endif
    snd_status_t alloc =
        memory->alloc(NULL, sizeof(shellcode64), SND_MEM_COMMIT | SND_MEM_RESERVE, SND_PAGE_EXECUTE_READWRITE, &pExec);
    if (SND_FAILED(alloc)) {
        log_err("Failed to allocate executable memory.");
        return alloc.code;
    }

    snd_memcpy(pExec, shellcode64, sizeof(shellcode64));

    log_info("Executing 64-bit shellcode at 0x%p via Heaven's Gate...", pExec);

    ULONGLONG    result = 0;
    snd_status_t st     = snd_hg_execute_64((ULONGLONG)(ULONG_PTR)pExec, 0, NULL, &result);

    if (SND_SUCCEEDED(st)) {
        log_ok("Heaven's Gate execution successful!");
        log_ok("64-bit code returned: 0x%llx (Expected: 0x1122334455667788)", result);
    } else {
        log_err("Heaven's Gate execution failed with status code: 0x%08X", st.code);
    }

    memory->free(pExec, 0, SND_MEM_RELEASE);
    return SND_SUCCEEDED(st) ? SND_SUCCESS : st.code;
}
