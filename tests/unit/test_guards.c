/*
 * Regression tests for the input/state guards hardened during review:
 * NULL handling in the syscall pipeline, setter clear semantics, cache
 * toggling, resolver failure without a clean NTDLL, and null-context
 * guards in the loader/injection/object-manager entry points.
 */

#include "test_util.h"

#include <sindri.h>

void snd_run_guard_tests(void) {
    snd_syscall_args_t args = {0};
    NTSTATUS           nt   = 0;

    PROGRESS("guard: invoke null args");
    CHECK(SND_FAILED(snd_syscall_invoke(0x1234, NULL, &nt)));
    CHECK(SND_FAILED(snd_syscall_invoke(0x1234, &args, NULL)));

    PROGRESS("guard: invoker clear");
    snd_syscall_set_invoker(NULL);
    snd_status_t st = snd_syscall_invoke(0x1234, &args, &nt);
    CHECK(st.code == SND_STATUS_SYSCALL_INVOKER_NOT_INITIALIZED);
    snd_syscall_set_invoker(snd_syscall_direct_invoke_asm); /* restore */

    PROGRESS("guard: finders clear");
    snd_syscall_set_gadget_finder(NULL);
    snd_syscall_set_spoof_finder(NULL);

    PROGRESS("guard: cache toggle");
    snd_syscall_cache_enable(TRUE);
    snd_syscall_cache_enable(FALSE);

    PROGRESS("guard: resolve without clean ntdll");
    snd_syscall_entry_t entry = {0};
    CHECK(SND_FAILED(snd_syscall_resolve(0x1234, &entry)));

    PROGRESS("guard: loader null context");
    CHECK(SND_FAILED(snd_ldr_pe_prepare_image(NULL)));

    PROGRESS("guard: injection cleanup null");
    snd_inj_cleanup(NULL);

    PROGRESS("guard: object manager null config");
    PVOID base = NULL;
    CHECK(SND_FAILED(snd_om_knowndll_map(NULL, L"ntdll.dll", &base)));
    PROGRESS("guard: object manager returned");
}
