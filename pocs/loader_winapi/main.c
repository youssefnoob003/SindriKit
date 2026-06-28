#include <sindri.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum number of -a arguments accepted on the command line. */
#define SND_MAX_CALL_ARGS 32

static void print_usage(const char *program_name, FILE *stream) {
    fprintf(stream,
            "Usage:\n"
            "  %s -f <payload_path> [-e <export_name>] [-a <arg>]...\n\n"
            "Options:\n"
            "  -f <payload_path>   Path to the PE payload (DLL or EXE).\n"
            "  -e <export_name>    Export to invoke (required for DLL payloads).\n"
            "  -a <arg>            Argument to pass to the export. Accepts decimal\n"
            "                      or hexadecimal (0x prefix) values. Repeat for\n"
            "                      multiple arguments (up to %d), in order.\n\n"
            "Notes:\n"
            "  - The payload type is auto-detected (DLL/EXE).\n"
            "  - For DLL payloads, -e is required and that export is called via the\n"
            "    runtime FFI bridge using any -a arguments supplied.\n"
            "  - For EXE payloads, -e and -a are ignored; the EXE entry point "
            "runs.\n",
            program_name, SND_MAX_CALL_ARGS);
}

int main(int argc, char *argv[]) {
    const char *file_path   = NULL;
    const char *export_name = NULL;

    /* Runtime argument array populated from -a flags. */
    UINT_PTR call_args[SND_MAX_CALL_ARGS];
    DWORD    call_argc = 0;

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

        if (strcmp(argv[i], "-e") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[-] Missing value for -e.\n");
                print_usage(argv[0], stderr);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            export_name = argv[++i];
            continue;
        }

        if (strcmp(argv[i], "-a") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[-] Missing value for -a.\n");
                print_usage(argv[0], stderr);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            if (call_argc >= SND_MAX_CALL_ARGS) {
                fprintf(stderr, "[-] Too many -a arguments (maximum: %d).\n", SND_MAX_CALL_ARGS);
                return SND_STATUS_INVALID_COMMAND_LINE_ARG;
            }
            /* ------------------------------------------------------------------
             * Try to parse as a numeric value (decimal or 0x-prefixed hex).
             * If parsing fails, treat the raw string itself as a pointer argument
             * — argv[] is valid for the process lifetime, so the char* is stable
             * across this synchronous call.
             * ------------------------------------------------------------------ */
            char              *endptr = NULL;
            const char        *raw    = argv[++i];
            unsigned long long parsed = strtoull(raw, &endptr, 0);

            if (endptr != raw && *endptr == '\0') {
                /* Successfully parsed as a number. */
                call_args[call_argc] = (UINT_PTR)parsed;
                printf("[*] arg[%lu] = 0x%llX (numeric)\n", (unsigned long)call_argc, parsed);
            } else {
                /* Not a valid number — pass as a string pointer. */
                call_args[call_argc] = (UINT_PTR)(const char *)raw;
                printf("[*] arg[%lu] = \"%s\" (string ptr: 0x%p)\n", (unsigned long)call_argc, raw,
                       (void *)(UINT_PTR)(const char *)raw);
            }
            call_argc++;
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

    snd_status_t status = SND_OK;

    snd_buffer_t     file_buf = {0};
    snd_ldr_pe_ctx_t ctx      = {0};

    ctx.mem_api = &snd_mem_win;
    ctx.mod_api = &snd_mod_win;

    printf("[+] Loading payload into memory: %s\n", file_path);
    status = snd_disk_buffer_load(file_path, &file_buf);
    if (SND_FAILED(status)) {
        goto cleanup;
    }

    ctx.raw_source = &file_buf;

    status = snd_ldr_pe_prepare_image(&ctx);
    if (SND_FAILED(status)) {
        goto cleanup;
    }

    if (ctx.pe.is_dll && export_name == NULL) {
        status = SND_ERR_CTX(SND_STATUS_MISSING_DLL_EXPORT_NAME,
                             "Export name is required for DLL payloads. Use -e <export_name>.");
        goto cleanup;
    }

    status = snd_ldr_pe_execute_image(&ctx);
    if (SND_FAILED(status)) {
        goto cleanup;
    }

    if (ctx.pe.is_dll && export_name != NULL) {
        /* ------------------------------------------------------------------
         * Resolve the export by name, then invoke it through the runtime FFI
         * bridge using the argument array built from -a flags.
         *
         * Type-punning strategy (C4152-safe):
         *   FARPROC -> (UINT_PTR) -> (PVOID)
         * The intermediate UINT_PTR cast makes the conversion an integer
         * operation, not a function-pointer conversion, so MSVC C4152 is
         * never triggered.
         * ------------------------------------------------------------------ */
        FARPROC dynamic_proc = NULL;
        status               = snd_ldr_pe_get_proc_address(&ctx, export_name, &dynamic_proc);
        if (SND_FAILED(status)) {
            goto cleanup;
        }

        printf("[+] Invoking export '%s' with %lu argument(s) via dynamic bridge.\n", export_name,
               (unsigned long)call_argc);

        PVOID    fn_ptr = (PVOID)(UINT_PTR)dynamic_proc;
        UINT_PTR retval = snd_ffi_execute(fn_ptr, call_argc, call_argc > 0 ? call_args : NULL);

        printf("[+] Export returned: 0x%p\n", (void *)retval);
    }

    printf("\n[+] Payload execution completed successfully.\n");

cleanup:
    snd_ldr_pe_detach_image(&ctx);
    snd_ldr_pe_free_mapped_image(&ctx);
    snd_buffer_free(&file_buf);

    if (SND_FAILED(status)) {
        snd_status_print(status);
        return status.code;
    }

    return status.code;
}
