#include <sindri.h>
#include <unified/cli.h>
#include <unified/commands.h>
#include <unified/common.h>
#include <unified/print.h>
#include <unified/syscall_cfg.h>

static void print_usage(const char *prog) {
    usage_header(prog, "load", "pe", "-f <payload_path> [-e <export_name>] [-a <arg>]... [--win|--nt|--sys]");
    poc_fprintf("\nOptions:\n");
    usage_opt("-f", "<path>", "Path to the PE payload (DLL or EXE).");
    usage_opt("-e", "<name>", "Export to invoke (required for DLL payloads).");
    usage_opt("-a", "<arg>", "Argument to pass to the export. Repeatable (max 32).");
    usage_opt("", "--win", "Use Win32 API (default).");
    usage_opt("", "--nt", "Use Native API (ntdll exports).");
    usage_opt("", "--sys", "Use direct syscalls (disk-loaded clean ntdll + SSN).");
    usage_opt("", "--invoke-direct", "Syscall invoker: direct assembly.");
    usage_opt("", "--invoke-indirect", "Syscall invoker: indirect assembly (default with --sys).");
    usage_opt("", "--invoke-spoofed", "Syscall invoker: spoofed / stack-duplicated assembly.");
    usage_opt("", "--resolve-scan", "SSN resolver: in-memory scan (default).");
    usage_opt("", "--resolve-sort", "SSN resolver: export-table sort.");
    usage_note("For DLL payloads, -e is required and the export is called via FFI. "
               "For EXE payloads, -e and -a are ignored; the EXE entry point runs.");
}

int cmd_load_pe(int argc, char *argv[], const char *prog) {
    const char   *file_path = NULL, *export_name = NULL;
    api_backend_t backend =
#if defined(SND_CRTLESS)
        API_NT;
#else
        API_WIN;
#endif
    syscall_style_t scfg = {.invoke = INVOKE_INDIRECT, .resolve_scan = 0, .resolve_sort = 0};
    UINT_PTR        call_args[SND_MAX_CALL_ARGS];
    DWORD           call_argc = 0;

    for (int i = 0; i < argc; i++) {
        const char *a = argv[i];
        if (poc_strcmp(a, "-h") == 0 || poc_strcmp(a, "--help") == 0) {
            print_usage(prog);
            return SND_SUCCESS;
        }
        if (poc_strcmp(a, "-f") == 0) {
            if (require_arg(argc, argv, i, "-f")) {
                print_usage(prog);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            file_path = argv[++i];
            continue;
        }
        if (poc_strcmp(a, "-e") == 0) {
            if (require_arg(argc, argv, i, "-e")) {
                print_usage(prog);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            export_name = argv[++i];
            continue;
        }
        if (poc_strcmp(a, "-a") == 0) {
            if (require_arg(argc, argv, i, "-a")) {
                print_usage(prog);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            if (call_argc >= SND_MAX_CALL_ARGS) {
                log_err("Too many -a arguments (max %d).", SND_MAX_CALL_ARGS);
                return SND_STATUS_INVALID_COMMAND_LINE_ARG;
            }
            const char        *raw    = argv[++i];
            char              *endptr = NULL;
            unsigned long long parsed = poc_strtoull(raw, &endptr, 0);
            if (endptr != raw && *endptr == '\0') {
                call_args[call_argc] = (UINT_PTR)parsed;
                log_info("arg[%lu] = 0x%llX (numeric)", (unsigned long)call_argc, parsed);
            } else {
                call_args[call_argc] = (UINT_PTR)raw;
                log_info("arg[%lu] = \"%s\" (string ptr: 0x%p)", (unsigned long)call_argc, raw,
                         (void *)call_args[call_argc]);
            }
            call_argc++;
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

    snd_status_t     st       = SND_OK;
    snd_buffer_t     file_buf = {0}, ntdll_buf = {0};
    snd_ldr_pe_ctx_t ctx = {0}, ntdll_ctx = {0};
    PVOID            ntdll = NULL;

    if (backend == API_SYS) {
        st = snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);
        if (SND_FAILED(st))
            goto cleanup;
        snd_ntdll_set_clean(ntdll);
        apply_syscall_style(&scfg);

        ctx.mem_api = &snd_mem_sys;
        ctx.mod_api = &snd_mod_nt;
        log_ok("Syscall mode active (disk-loaded ntdll).");
    } else if (backend == API_NT) {
        ctx.mem_api = &snd_mem_nt;
        ctx.mod_api = &snd_mod_nt;
        log_ok("Native API mode active (ntdll exports).");
    } else {
        ctx.mem_api = &unified_mem_win;
        ctx.mod_api = &unified_mod_win;
        log_ok("Win32 API mode active.");
    }

    log_ok("Loading PE payload: %s", file_path);
    st = unified_file_load(backend, file_path, &file_buf);
    if (SND_FAILED(st))
        goto cleanup;

    ctx.raw_source = &file_buf;
    st             = snd_ldr_pe_prepare_image(&ctx);
    if (SND_FAILED(st))
        goto cleanup;

    if (ctx.pe.is_dll && !export_name) {
        st = SND_ERR_CTX(SND_STATUS_MISSING_COMMAND_LINE_ARGS,
                         "Export name is required for DLL payloads. Use -e <export_name>.");
        goto cleanup;
    }

    st = snd_ldr_pe_execute_image(&ctx);
    if (SND_FAILED(st))
        goto cleanup;

    if (ctx.pe.is_dll && export_name) {
        FARPROC proc = NULL;
        st           = snd_ldr_pe_get_proc_address(&ctx, export_name, &proc);
        if (SND_FAILED(st))
            goto cleanup;

        log_ok("Invoking export '%s' with %lu argument(s).", export_name, (unsigned long)call_argc);
        UINT_PTR rv = snd_ffi_execute((PVOID)(UINT_PTR)proc, call_argc, call_argc ? call_args : NULL);
        log_ok("Export returned: 0x%p", (void *)rv);
    }

    log_ok("PE execution completed successfully.");

cleanup:
    snd_ldr_pe_detach_image(&ctx);
    snd_ldr_pe_free_mapped_image(&ctx);
    snd_buffer_free(&file_buf);
    snd_ldr_pe_detach_image(&ntdll_ctx);
    snd_ldr_pe_free_mapped_image(&ntdll_ctx);
    snd_buffer_free(&ntdll_buf);

    if (SND_FAILED(st)) {
        snd_status_print(st);
        return st.code;
    }
    return SND_SUCCESS;
}
