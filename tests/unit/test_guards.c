#include "test_util.h"

static void test_invoke_null_args(void) {
    snd_syscall_args_t args = {0};
    NTSTATUS           nt   = 0;

    CHECK(SND_FAILED(snd_syscall_invoke(0x1234, NULL, &nt)));
    CHECK(SND_FAILED(snd_syscall_invoke(0x1234, &args, NULL)));
}

static void test_invoker_clear(void) {
    snd_syscall_args_t args = {0};
    NTSTATUS           nt   = 0;

    snd_syscall_set_invoker(NULL);
    snd_status_t st = snd_syscall_invoke(0x1234, &args, &nt);
    CHECK(st.code == SND_STATUS_SYSCALL_INVOKER_NOT_INITIALIZED);
    snd_syscall_set_invoker(snd_syscall_direct_invoke_asm); /* restore */
}

static void test_finders_clear(void) {
    snd_syscall_set_gadget_finder(NULL);
    snd_syscall_set_spoof_finder(NULL);
}

static void test_cache_toggle(void) {
    snd_syscall_cache_enable(TRUE);
    snd_syscall_cache_enable(FALSE);
}

static void test_resolve_without_clean_ntdll(void) {
    snd_syscall_entry_t entry = {0};
    CHECK(SND_FAILED(snd_syscall_resolve(0x1234, &entry)));
}

static void test_loader_null_context(void) {
    CHECK(SND_FAILED(snd_ldr_pe_prepare_image(NULL)));
}

static void test_injection_cleanup_null(void) {
    snd_inj_cleanup(NULL);
}

static void test_object_manager_null_config(void) {
    PVOID base = NULL;
    CHECK(SND_FAILED(snd_om_knowndll_map(NULL, L"ntdll.dll", &base)));
}

void snd_test_register_guards(void) {
    snd_test_register("guards: syscall invoke null args", test_invoke_null_args);
    snd_test_register("guards: syscall invoker clear", test_invoker_clear);
    snd_test_register("guards: syscall finders clear", test_finders_clear);
    snd_test_register("guards: syscall cache toggle", test_cache_toggle);
    snd_test_register("guards: resolve without clean ntdll", test_resolve_without_clean_ntdll);
    snd_test_register("guards: loader null context", test_loader_null_context);
    snd_test_register("guards: injection cleanup null", test_injection_cleanup_null);
    snd_test_register("guards: object manager null config", test_object_manager_null_config);
}