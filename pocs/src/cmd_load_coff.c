#include <sindri.h>
#include <unified/backend.h>
#include <unified/cli.h>
#include <unified/commands.h>
#include <unified/print.h>

static void print_usage(const char *prog) {
    usage_header(prog, "load", "coff", "-f <payload_path> [-e <entry_point>] [-a <arg>]... [--win|--nt|--sys]");
    poc_fprintf("\nOptions:\n");
    usage_opt("-f", "<path>", "Path to the COFF object file (.obj).");
    usage_opt("-e", "<name>", "Name of the function to execute (default: 'go').");
    usage_opt("-a", "<arg>", "Argument to pass to the BOF. Repeatable (max 32).");
    usage_backend_flags();
    usage_syscall_flags();
    poc_fprintf("\nDefaults: %s backend, indirect invoker, scan resolver with sort fallback, syscall cache off.\n",
                SND_POC_DEFAULT_BACKEND_NAME);
}

int cmd_load_coff(int argc, char *argv[], const char *prog) {
    const char     *file_path = NULL, *entry_name = "go";
    api_backend_t   backend = SND_POC_DEFAULT_BACKEND;
    syscall_style_t scfg    = {.invoke = INVOKE_INDIRECT, .resolve_scan = 0, .resolve_sort = 0};
    UINT_PTR        call_args[SND_MAX_CALL_ARGS];
    DWORD           call_argc = 0;

    for (int i = 0; i < argc; i++) {
        const char *a = argv[i];
        if (poc_strcmp(a, "-h") == 0 || poc_strcmp(a, "--help") == 0) {
            print_usage(prog);
            return SND_SUCCESS;
        }
        if (poc_strcmp(a, "-f") == 0) {
            if (require_arg(argc, i, "-f")) {
                print_usage(prog);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            file_path = argv[++i];
            continue;
        }
        if (poc_strcmp(a, "-e") == 0) {
            if (require_arg(argc, i, "-e")) {
                print_usage(prog);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            entry_name = argv[++i];
            continue;
        }
        if (poc_strcmp(a, "-a") == 0) {
            snd_status_t parse_status = parse_call_arg(argc, argv, &i, call_args, &call_argc);
            if (SND_FAILED(parse_status)) {
                print_usage(prog);
                return parse_status.code;
            }
            continue;
        }
        if (!parse_backend(argc, argv, &i, &backend))
            continue;
        if (!parse_syscall_style(argc, argv, &i, &scfg))
            continue;

        log_err("Unknown argument: %s", a);
        print_usage(prog);
        return SND_STATUS_INVALID_COMMAND_LINE_ARG;
    }

    if (!file_path) {
        log_err("-f <payload_path> is required.");
        print_usage(prog);
        return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
    }

    snd_status_t       st       = SND_OK;
    snd_buffer_t       file_buf = {0};
    snd_ldr_coff_ctx_t ctx      = {0};
    unified_backend_t  be       = {0};

    st = unified_backend_init(backend, &scfg, &be);
    if (SND_FAILED(st))
        goto cleanup;

    log_ok("%s backend active.", unified_backend_name(backend));

    ctx.mem_api = be.mem_api;
    ctx.mod_api = be.mod_api;

    log_ok("Loading COFF payload: %s", file_path);
    st = unified_file_load(&be, file_path, &file_buf);
    if (SND_FAILED(st))
        goto cleanup;

    ctx.raw_source = &file_buf;
    st             = snd_ldr_coff_load(&ctx);
    if (SND_FAILED(st))
        goto cleanup;

    log_ok("COFF loaded. Resolving entry point '%s' with %lu argument(s).", entry_name, (unsigned long)call_argc);

    char *bof_args    = call_argc > 0 ? (char *)call_args : NULL;
    int   bof_arg_len = (int)(call_argc * sizeof(UINT_PTR));

    st = snd_ldr_coff_execute_image(&ctx, entry_name, bof_args, bof_arg_len);
    if (SND_FAILED(st))
        goto cleanup;

    log_ok("COFF execution completed successfully.");

cleanup:
    snd_ldr_coff_free_mapped_image(&ctx);
    snd_buffer_free(&file_buf);

    if (SND_FAILED(st)) {
        snd_status_print(st);
        return st.code;
    }
    return SND_SUCCESS;
}
