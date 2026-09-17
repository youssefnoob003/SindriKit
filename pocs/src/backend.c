#include <unified/backend.h>

void apply_syscall_style(const syscall_style_t *cfg) {
    snd_syscall_set_spoof_finder(NULL);
    snd_syscall_cache_enable(cfg->cache ? TRUE : FALSE);

    if (cfg->invoke == INVOKE_DIRECT) {
        snd_syscall_set_invoker(snd_syscall_direct_invoke_asm);
    } else if (cfg->invoke == INVOKE_SPOOFED) {
        snd_syscall_set_spoof_finder(snd_syscall_find_spoof_scan);
        snd_syscall_set_invoker(snd_syscall_spoofed_invoke_asm);
    } else {
        snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
    }

    snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);

    // "scan -> sort" is the implicit default when neither resolver flag is set.
    if (cfg->resolve_sort && !cfg->resolve_scan) {
        snd_syscall_set_resolver(snd_syscall_resolve_ssn_sort);
        return;
    }

    snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
    if (cfg->resolve_sort || !cfg->resolve_scan) {
        snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
    }
}

snd_status_t unified_backend_init(api_backend_t backend, const syscall_style_t *scfg, unified_backend_t *out) {
    SND_CHECK_NULL(out);

    unified_backend_t bound = {.backend = backend};

    switch (backend) {
    case API_SYS: {
        PVOID ntdll = NULL;
        SND_TRY(snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll));
        SND_TRY(snd_ntdll_set_clean(ntdll));
        apply_syscall_style(scfg);

        bound.file_api   = &snd_file_sys;
        bound.mem_api    = &snd_mem_sys;
        bound.mod_api    = &snd_mod_nt;
        bound.proc_api   = &snd_proc_sys;
        bound.thread_api = &snd_thread_sys;
        break;
    }
    case API_NT:
        bound.file_api   = &snd_file_nt;
        bound.mem_api    = &snd_mem_nt;
        bound.mod_api    = &snd_mod_nt;
        bound.proc_api   = &snd_proc_nt;
        bound.thread_api = &snd_thread_nt;
        break;

    case API_WIN:
#if defined(SND_CRTLESS)
        return SND_ERR_CTX(SND_STATUS_INVALID_COMMAND_LINE_ARG, "Win32 backend is unavailable in CRT-less mode.");
#else
        bound.file_api   = &snd_file_win;
        bound.mem_api    = &snd_mem_win;
        bound.mod_api    = &snd_mod_win;
        bound.proc_api   = &snd_proc_win;
        bound.thread_api = &snd_thread_win;
        break;
#endif

    default:
        return SND_ERR_CTX(SND_STATUS_INVALID_COMMAND_LINE_ARG, "Unknown backend.");
    }

    *out = bound;
    return SND_OK;
}

snd_status_t unified_file_load(const unified_backend_t *backend, const char *path, snd_buffer_t *out_buffer) {
    SND_CHECK_NULL(backend, backend->file_api, path, out_buffer);
    return backend->file_api->load(path, out_buffer);
}
