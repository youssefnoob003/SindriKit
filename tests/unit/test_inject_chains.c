#include "test_util.h"

#include <sindri/injection/apc/chain.h>
#include <sindri/injection/classic/chain.h>
#include <sindri/injection/common/context.h>
#include <sindri/injection/hijack/chain.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/primitives/os_api.h>
#include <stdio.h>
#include <string.h>

#define FAKE_BASE        ((PVOID)0x10000000u)
#define FAKE_REMOTE_SIZE 0x1000u
#define FAKE_PROC        ((HANDLE)(ULONG_PTR)0xAAA1u)
#define FAKE_THREAD      ((HANDLE)(ULONG_PTR)0xAAA2u)
#define FAKE_PID         0x1234u

static const unsigned char g_payload[64] = {0x90};

static unsigned char g_fake_remote[FAKE_REMOTE_SIZE];
static const char   *g_log[64];
static int           g_log_count;
static int           g_seq;
static int           g_fail_at; /* 1-based callback sequence to fail at, -1 = none */
static int           g_partial_write;

static PVOID     g_last_start_address;
static ULONG_PTR g_last_ip;
static HANDLE    g_last_apc_thread;
static ULONG_PTR g_last_frame[3];
static SIZE_T    g_last_frame_size;

static void fake_reset(void) {
    memset(g_fake_remote, 0, sizeof(g_fake_remote));
    g_log_count          = 0;
    g_seq                = 0;
    g_fail_at            = -1;
    g_partial_write      = 0;
    g_last_start_address = NULL;
    g_last_ip            = 0;
    g_last_apc_thread    = NULL;
    memset(g_last_frame, 0, sizeof(g_last_frame));
    g_last_frame_size = 0;
}

static void log_call(const char *name) {
    if (g_log_count < (int)(sizeof(g_log) / sizeof(g_log[0]))) {
        g_log[g_log_count++] = name;
    }
}

static int log_has(const char *name) {
    for (int i = 0; i < g_log_count; i++) {
        if (g_log[i] != NULL && strcmp(g_log[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

static int log_ordered(const char *const *names, int count) {
    int pos = 0;
    for (int i = 0; i < count; i++) {
        int found = 0;
        for (; pos < g_log_count; pos++) {
            if (g_log[pos] != NULL && strcmp(g_log[pos], names[i]) == 0) {
                found = 1;
                pos++;
                break;
            }
        }
        if (!found) {
            return 0;
        }
    }
    return 1;
}

/* Self-contained assertion (not nested inside CHECK) so multi-line
 * invocations remain a single, well-terminated macro call. */
#define CHECK_LOG_ORDERED(...)                                                                                         \
    do {                                                                                                               \
        const char *_snd_seq[] = {__VA_ARGS__};                                                                        \
        if (!log_ordered(_snd_seq, (int)(sizeof(_snd_seq) / sizeof(_snd_seq[0])))) {                                   \
            snd_test_fail(__FILE__, __LINE__, "call order: " #__VA_ARGS__, NULL);                                      \
        }                                                                                                              \
    } while (0)

static snd_status_t maybe_fail(const char *name) {
    g_seq++;
    if (g_fail_at == g_seq) {
        return SND_ERR(SND_STATUS_UNSUPPORTED);
    }
    log_call(name);
    return SND_OK;
}

/* ── fake process API ────────────────────────────────────────────────────── */

static snd_status_t WINAPI fake_create_process(const snd_process_api_t *api, const wchar_t *image_path,
                                               const wchar_t *command_line, HANDLE *out_process, HANDLE *out_thread) {
    (void)api;
    (void)image_path;
    (void)command_line;
    snd_status_t st = maybe_fail("create_process");
    if (SND_FAILED(st)) {
        return st;
    }
    *out_process = FAKE_PROC;
    *out_thread  = FAKE_THREAD;
    return SND_OK;
}

static snd_status_t WINAPI fake_open_process(DWORD pid, DWORD desired_access, HANDLE *out_process) {
    (void)pid;
    (void)desired_access;
    snd_status_t st = maybe_fail("open_process");
    if (SND_FAILED(st)) {
        return st;
    }
    *out_process = FAKE_PROC;
    return SND_OK;
}

static snd_status_t WINAPI fake_alloc_remote(HANDLE process, SIZE_T size, DWORD allocation_type, DWORD protect,
                                             PVOID *out_address) {
    (void)process;
    (void)size;
    (void)allocation_type;
    (void)protect;
    snd_status_t st = maybe_fail("alloc_remote");
    if (SND_FAILED(st)) {
        return st;
    }
    *out_address = FAKE_BASE;
    return SND_OK;
}

static snd_status_t WINAPI fake_free_remote(HANDLE process, PVOID base_address, SIZE_T size, DWORD free_type) {
    (void)process;
    (void)base_address;
    (void)size;
    (void)free_type;
    return maybe_fail("free_remote");
}

static snd_status_t WINAPI fake_write_remote(HANDLE process, PVOID base_address, const void *buffer, SIZE_T size,
                                             SIZE_T *bytes_written) {
    (void)process;
    snd_status_t st = maybe_fail("write_remote");
    if (SND_FAILED(st)) {
        return st;
    }
    *bytes_written = g_partial_write && size > 0 ? size - 1 : size;
    if (base_address == FAKE_BASE && size <= sizeof(g_fake_remote)) {
        memcpy(g_fake_remote, buffer, size);
    }
    if (base_address != FAKE_BASE && size <= sizeof(g_last_frame)) {
        memcpy(g_last_frame, buffer, size);
        g_last_frame_size = size;
    }
    return SND_OK;
}

static snd_status_t WINAPI fake_terminate_process(HANDLE process, UINT exit_code) {
    (void)process;
    (void)exit_code;
    return maybe_fail("terminate_process");
}

static snd_status_t WINAPI fake_protect_remote(HANDLE process, PVOID base_address, SIZE_T size, DWORD new_protect,
                                               DWORD *old_protect) {
    (void)process;
    (void)base_address;
    (void)size;
    (void)new_protect;
    snd_status_t st = maybe_fail("protect_remote");
    if (SND_FAILED(st)) {
        return st;
    }
    *old_protect = SND_PAGE_READWRITE;
    return SND_OK;
}

static snd_status_t WINAPI fake_create_remote_thread(HANDLE process, PVOID start_address, PVOID parameter,
                                                     HANDLE *out_thread) {
    (void)process;
    (void)parameter;
    snd_status_t st = maybe_fail("create_remote_thread");
    if (SND_FAILED(st)) {
        return st;
    }
    g_last_start_address = start_address;
    *out_thread          = FAKE_THREAD;
    return SND_OK;
}

static snd_status_t WINAPI fake_close_handle(HANDLE handle) {
    (void)handle;
    return maybe_fail("close_handle");
}

/* ── fake thread API ─────────────────────────────────────────────────────── */

static snd_status_t WINAPI fake_queue_apc(HANDLE thread, PVOID apc_routine, PVOID apc_argument) {
    (void)apc_routine;
    (void)apc_argument;
    snd_status_t st = maybe_fail("queue_apc");
    if (SND_FAILED(st)) {
        return st;
    }
    g_last_apc_thread = thread;
    return SND_OK;
}

static snd_status_t WINAPI fake_resume_thread(HANDLE thread) {
    (void)thread;
    return maybe_fail("resume_thread");
}

static snd_status_t WINAPI fake_get_context(HANDLE thread, SND_THREAD_REGISTERS *out_regs) {
    (void)thread;
    snd_status_t st = maybe_fail("get_context");
    if (SND_FAILED(st)) {
        return st;
    }
    memset(out_regs, 0, sizeof(*out_regs));
    out_regs->sp     = (ULONG_PTR)0x1000; /* 16-byte aligned */
    out_regs->rflags = 0x0202;            /* IF */
    return SND_OK;
}

static snd_status_t WINAPI fake_set_context(HANDLE thread, const SND_THREAD_REGISTERS *in_regs) {
    (void)thread;
    snd_status_t st = maybe_fail("set_context");
    if (SND_FAILED(st)) {
        return st;
    }
    g_last_ip = in_regs->ip;
    return SND_OK;
}

/* ── context helpers ─────────────────────────────────────────────────────── */

static snd_process_api_t g_fake_proc = {
    .create_process       = fake_create_process,
    .open_process         = fake_open_process,
    .alloc_remote         = fake_alloc_remote,
    .free_remote          = fake_free_remote,
    .write_remote         = fake_write_remote,
    .protect_remote       = fake_protect_remote,
    .create_remote_thread = fake_create_remote_thread,
    .terminate_process    = fake_terminate_process,
    .close_handle         = fake_close_handle,
};

static snd_thread_api_t g_fake_thread = {
    .queue_apc     = fake_queue_apc,
    .resume_thread = fake_resume_thread,
    .get_context   = fake_get_context,
    .set_context   = fake_set_context,
    .close_handle  = fake_close_handle,
};

static void init_ctx(snd_inj_ctx_t *ctx, snd_buffer_t *payload, const snd_process_api_t *proc,
                     const snd_thread_api_t *thread) {
    memset(ctx, 0, sizeof(*ctx));
    payload->data          = (void *)g_payload;
    payload->size          = sizeof(g_payload);
    ctx->payload           = payload;
    ctx->proc_api          = proc;
    ctx->thread_api        = thread;
    ctx->target_pid        = FAKE_PID;
    ctx->target_image_path = L"fake_target.exe";
}

static void test_common_staging_sequence(void) {
    snd_inj_ctx_t ctx;
    snd_buffer_t  payload;
    fake_reset();
    init_ctx(&ctx, &payload, &g_fake_proc, &g_fake_thread);

    SND_CHECK_SUCCEEDED(snd_inj_create_suspended_target(&ctx));
    SND_CHECK_SUCCEEDED(snd_inj_alloc_remote(&ctx));
    SND_CHECK_SUCCEEDED(snd_inj_write_payload(&ctx));
    SND_CHECK_SUCCEEDED(snd_inj_set_protections(&ctx));
    CHECK(ctx.stage == SND_INJ_STAGE_PROTECTIONS_SET);
    CHECK_LOG_ORDERED("create_process", "alloc_remote", "write_remote", "protect_remote");

    snd_inj_cleanup(&ctx);
    CHECK(log_has("close_handle"));
    CHECK(log_has("free_remote"));
}

/* ── classic ─────────────────────────────────────────────────────────────── */

static void test_classic_shell_sequence(void) {
    snd_inj_ctx_t ctx;
    snd_buffer_t  payload;
    fake_reset();
    init_ctx(&ctx, &payload, &g_fake_proc, NULL);

    SND_CHECK_SUCCEEDED(snd_inj_classic_shell(&ctx));
    CHECK(ctx.stage == SND_INJ_STAGE_EXECUTED);
    CHECK_LOG_ORDERED("open_process", "alloc_remote", "write_remote", "protect_remote", "create_remote_thread");
    CHECK(ctx.remote_base == FAKE_BASE);
    CHECK(ctx.remote_size == sizeof(g_payload));
    CHECK(memcmp(g_fake_remote, g_payload, sizeof(g_payload)) == 0);
    CHECK(g_last_start_address == FAKE_BASE);
    CHECK(ctx.remote_thread == FAKE_THREAD);

    snd_inj_cleanup(&ctx);
    CHECK(log_has("close_handle"));
    CHECK(!log_has("free_remote"));
    CHECK(ctx.target_process == NULL);
}

static void test_classic_shell_stage_guard(void) {
    snd_inj_ctx_t ctx;
    snd_buffer_t  payload;
    fake_reset();
    init_ctx(&ctx, &payload, &g_fake_proc, NULL);

    /* Run once (reaches EXECUTED), then a fresh chain must refuse a stage !=
     * UNINITIALIZED context. */
    SND_CHECK_SUCCEEDED(snd_inj_classic_shell(&ctx));
    g_seq       = 0;
    g_log_count = 0;
    SND_CHECK_STATUS(snd_inj_classic_shell(&ctx), SND_STATUS_INVALID_STAGE);
    snd_inj_cleanup(&ctx);
}

static void test_classic_shell_error_propagation(void) {
    snd_inj_ctx_t ctx;
    snd_buffer_t  payload;
    fake_reset();
    init_ctx(&ctx, &payload, &g_fake_proc, NULL);

    /* Fail at protect_remote (4th call): the chain must fail and cleanup must
     * reset the context safely. */
    g_fail_at = 4;
    SND_CHECK_STATUS(snd_inj_classic_shell(&ctx), SND_STATUS_UNSUPPORTED);
    CHECK(ctx.stage == SND_INJ_STAGE_UNINITIALIZED);
    snd_inj_cleanup(&ctx);
    CHECK(ctx.target_process == NULL);
}

static void test_classic_null_table(void) {
    snd_inj_ctx_t ctx = {0};
    SND_CHECK_STATUS(snd_inj_classic_shell(NULL), SND_STATUS_NULL_POINTER);
    SND_CHECK_STATUS(snd_inj_classic_shell(&ctx), SND_STATUS_NULL_POINTER);
}

static void test_hijack_coff_rejects_negative_arg_len(void) {
    snd_ldr_coff_ctx_t ldr = {0};
    snd_inj_ctx_t      inj = {0};
    snd_buffer_t       raw = {0};
    ldr.raw_source         = &raw;

    SND_CHECK_STATUS(snd_inj_hijack_coff(&ldr, &inj, NULL, NULL, -1), SND_STATUS_INVALID_PARAMETERS_COMBINATION);
}

/* ── APC ─────────────────────────────────────────────────────────────────── */

static void test_apc_shell_sequence(void) {
    snd_inj_ctx_t ctx;
    snd_buffer_t  payload;
    fake_reset();
    init_ctx(&ctx, &payload, &g_fake_proc, &g_fake_thread);

    SND_CHECK_SUCCEEDED(snd_inj_apc_shell(&ctx));
    CHECK(ctx.stage == SND_INJ_STAGE_EXECUTED);
    CHECK_LOG_ORDERED("create_process", "alloc_remote", "write_remote", "protect_remote", "queue_apc", "resume_thread");
    CHECK(ctx.target_process == FAKE_PROC);
    CHECK(ctx.remote_thread == FAKE_THREAD);
    CHECK(g_last_apc_thread == FAKE_THREAD);
    CHECK(memcmp(g_fake_remote, g_payload, sizeof(g_payload)) == 0);

    snd_inj_cleanup(&ctx);
    CHECK(log_has("close_handle"));
}

/* ── hijack ──────────────────────────────────────────────────────────────── */

#if defined(_WIN64)
static void test_hijack_shell_sequence(void) {
    snd_inj_ctx_t ctx;
    snd_buffer_t  payload;
    fake_reset();
    init_ctx(&ctx, &payload, &g_fake_proc, &g_fake_thread);

    /* Graceful return: the thunk is written between get_context and
     * set_context (the payload write is the other write_remote). */
    ctx.return_policy = SND_INJ_RETURN_GRACEFUL;
    SND_CHECK_SUCCEEDED(snd_inj_hijack_shell(&ctx));
    CHECK(ctx.stage == SND_INJ_STAGE_EXECUTED);
    CHECK_LOG_ORDERED("create_process", "alloc_remote", "write_remote", "protect_remote", "get_context", "write_remote",
                      "set_context", "resume_thread");
    CHECK(g_last_ip == (ULONG_PTR)FAKE_BASE);
    CHECK(g_last_frame_size == sizeof(PVOID));
    CHECK(g_last_frame[0] != 0);

    snd_inj_cleanup(&ctx);
}

static void test_hijack_shell_no_thunk(void) {
    snd_inj_ctx_t ctx;
    snd_buffer_t  payload;
    fake_reset();
    init_ctx(&ctx, &payload, &g_fake_proc, &g_fake_thread);

    SND_CHECK_SUCCEEDED(snd_inj_hijack_shell(&ctx));
    CHECK(ctx.stage == SND_INJ_STAGE_EXECUTED);
    CHECK_LOG_ORDERED("create_process", "alloc_remote", "write_remote", "protect_remote", "get_context", "set_context",
                      "resume_thread");
    CHECK(g_last_frame_size == 0);
    snd_inj_cleanup(&ctx);
}

static void test_hijack_partial_write_cleanup(void) {
    snd_inj_ctx_t ctx;
    snd_buffer_t  payload;
    fake_reset();
    init_ctx(&ctx, &payload, &g_fake_proc, &g_fake_thread);
    g_partial_write = 1;

    SND_CHECK_STATUS(snd_inj_hijack_shell(&ctx), SND_STATUS_PROCESS_REMOTE_WRITE_FAILED);
    CHECK(ctx.stage == SND_INJ_STAGE_UNINITIALIZED);
    snd_inj_cleanup(&ctx);
    CHECK(log_has("terminate_process"));
}

static void test_hijack_resume_failure_is_not_retryable(void) {
    snd_inj_ctx_t ctx;
    snd_buffer_t  payload;
    fake_reset();
    init_ctx(&ctx, &payload, &g_fake_proc, &g_fake_thread);
    ctx.return_policy = SND_INJ_RETURN_GRACEFUL;
    g_fail_at         = 8; /* resume_thread, after context has been applied */

    SND_CHECK_STATUS(snd_inj_hijack_shell(&ctx), SND_STATUS_UNSUPPORTED);
    CHECK(ctx.stage == SND_INJ_STAGE_UNINITIALIZED);
    snd_inj_cleanup(&ctx);
    CHECK(log_has("terminate_process"));
}
#else
static void test_hijack_shell_x86_sequence(void) {
    snd_inj_ctx_t ctx;
    snd_buffer_t  payload;
    fake_reset();
    init_ctx(&ctx, &payload, &g_fake_proc, &g_fake_thread);

    SND_CHECK_SUCCEEDED(snd_inj_hijack_shell(&ctx));
    CHECK_LOG_ORDERED("create_process", "alloc_remote", "write_remote", "protect_remote", "get_context", "set_context",
                      "resume_thread");
    CHECK(ctx.stage == SND_INJ_STAGE_EXECUTED);
    CHECK(g_last_frame_size == 0);
    snd_inj_cleanup(&ctx);
}
#endif

void snd_test_register_inject_chains(void) {
    snd_test_register("inject chains: classic sequence", test_classic_shell_sequence);
    snd_test_register("inject chains: classic stage guard", test_classic_shell_stage_guard);
    snd_test_register("inject chains: classic error propagation", test_classic_shell_error_propagation);
    snd_test_register("inject chains: classic null table", test_classic_null_table);
    snd_test_register("inject common: staging sequence", test_common_staging_sequence);
    snd_test_register("inject chains: hijack COFF negative arg length", test_hijack_coff_rejects_negative_arg_len);
    snd_test_register("inject chains: apc sequence", test_apc_shell_sequence);
#if defined(_WIN64)
    snd_test_register("inject chains: hijack sequence", test_hijack_shell_sequence);
    snd_test_register("inject chains: hijack no thunk", test_hijack_shell_no_thunk);
    snd_test_register("inject chains: hijack partial write cleanup", test_hijack_partial_write_cleanup);
    snd_test_register("inject chains: hijack resume failure", test_hijack_resume_failure_is_not_retryable);
#else
    snd_test_register("inject chains: hijack x86 sequence", test_hijack_shell_x86_sequence);
#endif
}
