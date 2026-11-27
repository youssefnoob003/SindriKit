#include "coff_builder.h"
#include "test_util.h"

#include <sindri/injection/apc/chain.h>
#include <sindri/injection/classic/chain.h>
#include <sindri/injection/common/context.h>
#include <sindri/injection/hijack/chain.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/loaders/coff/status.h>
#include <sindri/loaders/pe/status.h>
#include <sindri/primitives/os_api.h>
#include <stdio.h>
#include <stdlib.h>
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

typedef struct {
    PVOID  addr;
    SIZE_T size;
    DWORD  protect;
} fake_protect_record_t;

#define FAKE_MAX_PROTECTS 32

static PVOID                 g_last_start_address;
static ULONG_PTR             g_last_ip;
static HANDLE                g_last_apc_thread;
static ULONG_PTR             g_last_frame[3];
static SIZE_T                g_last_frame_size;
static SIZE_T                g_last_alloc_size;
static DWORD                 g_last_local_alloc_protect;
static fake_protect_record_t g_protects[FAKE_MAX_PROTECTS];
static int                   g_protect_count;

static void fake_reset(void) {
    memset(g_fake_remote, 0, sizeof(g_fake_remote));
    g_log_count                = 0;
    g_seq                      = 0;
    g_fail_at                  = -1;
    g_partial_write            = 0;
    g_last_start_address       = NULL;
    g_last_ip                  = 0;
    g_last_apc_thread          = NULL;
    g_last_alloc_size          = 0;
    g_last_local_alloc_protect = 0;
    g_protect_count            = 0;
    memset(g_protects, 0, sizeof(g_protects));
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
    (void)allocation_type;
    (void)protect;
    snd_status_t st = maybe_fail("alloc_remote");
    if (SND_FAILED(st)) {
        return st;
    }
    g_last_alloc_size = size;
    *out_address      = FAKE_BASE;
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
    snd_status_t st = maybe_fail("protect_remote");
    if (SND_FAILED(st)) {
        return st;
    }
    if (g_protect_count < FAKE_MAX_PROTECTS) {
        g_protects[g_protect_count].addr    = base_address;
        g_protects[g_protect_count].size    = size;
        g_protects[g_protect_count].protect = new_protect;
        g_protect_count++;
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

/* ── fake local loader APIs ──────────────────────────────────────────────── */

static snd_status_t WINAPI fake_local_alloc(LPVOID address, SIZE_T size, DWORD allocation_type, DWORD protect,
                                            LPVOID *out_address) {
    (void)address;
    (void)allocation_type;
    if (!out_address || size == 0) {
        return SND_ERR(SND_STATUS_ALLOC_FAILED);
    }
    g_last_local_alloc_protect = protect;
    *out_address               = calloc(1, size);
    return *out_address ? SND_OK : SND_ERR(SND_STATUS_ALLOC_FAILED);
}

static snd_status_t WINAPI fake_local_free(LPVOID address, SIZE_T size, DWORD free_type) {
    (void)size;
    (void)free_type;
    free(address);
    return SND_OK;
}

static snd_status_t WINAPI fake_local_protect(LPVOID address, SIZE_T size, DWORD new_protect, DWORD *old_protect) {
    (void)address;
    (void)size;
    (void)new_protect;
    if (old_protect) {
        *old_protect = SND_PAGE_READWRITE;
    }
    return SND_OK;
}

static snd_status_t WINAPI fake_mod_load(const char *module_name, HMODULE *out_module) {
    (void)module_name;
    *out_module = (HMODULE)(ULONG_PTR)0x77770000u;
    return SND_OK;
}

static snd_status_t WINAPI fake_mod_get_proc(HMODULE hModule, const char *proc_name, FARPROC *out_proc) {
    (void)hModule;
    (void)proc_name;
    *out_proc = (FARPROC)(ULONG_PTR)0x77771234u;
    return SND_OK;
}

static const snd_memory_api_t g_fake_mem = {
    .alloc   = fake_local_alloc,
    .free    = fake_local_free,
    .protect = fake_local_protect,
};

static const snd_module_api_t g_fake_mod = {
    .load_library     = fake_mod_load,
    .get_proc_address = fake_mod_get_proc,
};

static void build_multi_section_pe(unsigned char *buf, size_t buf_size, DWORD entry_rva) {
    memset(buf, 0, buf_size);

    PSND_IMAGE_DOS_HEADER dos = (PSND_IMAGE_DOS_HEADER)buf;
    dos->e_magic              = SND_IMAGE_DOS_SIGNATURE;
    dos->e_lfanew             = 0x80;

    *(DWORD *)(buf + 0x80)    = SND_IMAGE_NT_SIGNATURE;
    PSND_IMAGE_FILE_HEADER fh = (PSND_IMAGE_FILE_HEADER)(buf + 0x80 + sizeof(DWORD));
    fh->NumberOfSections      = 3;
    fh->Characteristics       = SND_IMAGE_FILE_DLL;

#if defined(_WIN64)
    fh->Machine              = SND_IMAGE_FILE_MACHINE_AMD64;
    fh->SizeOfOptionalHeader = sizeof(SND_IMAGE_OPTIONAL_HEADER64);
    PSND_IMAGE_OPTIONAL_HEADER64 opt =
        (PSND_IMAGE_OPTIONAL_HEADER64)(buf + 0x80 + sizeof(DWORD) + sizeof(SND_IMAGE_FILE_HEADER));
    opt->Magic = SND_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
#else
    fh->Machine              = SND_IMAGE_FILE_MACHINE_I386;
    fh->SizeOfOptionalHeader = sizeof(SND_IMAGE_OPTIONAL_HEADER32);
    PSND_IMAGE_OPTIONAL_HEADER32 opt =
        (PSND_IMAGE_OPTIONAL_HEADER32)(buf + 0x80 + sizeof(DWORD) + sizeof(SND_IMAGE_FILE_HEADER));
    opt->Magic = SND_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
#endif
    opt->ImageBase           = (ULONG_PTR)FAKE_BASE;
    opt->SectionAlignment    = 0x1000;
    opt->FileAlignment       = 0x200;
    opt->SizeOfHeaders       = 0x400;
    opt->SizeOfImage         = 0x4000;
    opt->AddressOfEntryPoint = entry_rva;
    opt->NumberOfRvaAndSizes = SND_IMAGE_NUMBEROF_DIRECTORY_ENTRIES;

    PSND_IMAGE_SECTION_HEADER secs =
        (PSND_IMAGE_SECTION_HEADER)(buf + 0x80 + sizeof(DWORD) + sizeof(SND_IMAGE_FILE_HEADER) +
                                    fh->SizeOfOptionalHeader);

    memcpy(secs[0].Name, ".text", 5);
    secs[0].Misc.VirtualSize = 0x200;
    secs[0].VirtualAddress   = 0x1000;
    secs[0].SizeOfRawData    = 0x200;
    secs[0].PointerToRawData = 0x400;
    secs[0].Characteristics  = SND_IMAGE_SCN_MEM_EXECUTE | SND_IMAGE_SCN_MEM_READ;

    memcpy(secs[1].Name, ".rdata", 6);
    secs[1].Misc.VirtualSize = 0x200;
    secs[1].VirtualAddress   = 0x2000;
    secs[1].SizeOfRawData    = 0x200;
    secs[1].PointerToRawData = 0x600;
    secs[1].Characteristics  = SND_IMAGE_SCN_MEM_READ;

    memcpy(secs[2].Name, ".data", 5);
    secs[2].Misc.VirtualSize = 0x200;
    secs[2].VirtualAddress   = 0x3000;
    secs[2].SizeOfRawData    = 0x200;
    secs[2].PointerToRawData = 0x800;
    secs[2].Characteristics  = SND_IMAGE_SCN_MEM_READ | SND_IMAGE_SCN_MEM_WRITE;
}

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

static snd_status_t empty_protect_plan(snd_memory_protect_region_cb apply_region, void *apply_ctx,
                                         const void *plan_ctx) {
    (void)apply_region;
    (void)apply_ctx;
    (void)plan_ctx;
    return SND_OK;
}

static snd_status_t oob_protect_plan(snd_memory_protect_region_cb apply_region, void *apply_ctx, const void *plan_ctx) {
    const snd_inj_ctx_t *ctx = (const snd_inj_ctx_t *)plan_ctx;
    return apply_region(0x20, ctx->remote_size, SND_PAGE_READONLY, apply_ctx);
}

static void test_prepare_pe_per_section_protections(void) {
    unsigned char raw_pe[0xA00];
    build_multi_section_pe(raw_pe, sizeof(raw_pe), 0x1000);

    snd_buffer_t     raw_buf = {.data = raw_pe, .size = sizeof(raw_pe)};
    snd_ldr_pe_ctx_t ldr     = {0};
    snd_inj_ctx_t    inj     = {0};

    fake_reset();
    ldr.raw_source = &raw_buf;
    ldr.mem_api    = &g_fake_mem;
    ldr.mod_api    = &g_fake_mod;

    inj.target_pid = FAKE_PID;
    inj.proc_api   = &g_fake_proc;

    SND_CHECK_SUCCEEDED(snd_inj_classic_pe(&ldr, &inj));
    CHECK_EQ_INT(g_protect_count, 4);
    CHECK_EQ_PTR(g_protects[0].addr, FAKE_BASE);
    CHECK_EQ_U32((DWORD)g_protects[0].size, 0x1000u);
    CHECK_EQ_U32(g_protects[0].protect, SND_PAGE_READONLY);

    CHECK_EQ_PTR(g_protects[1].addr, SND_PTR_ADD(FAKE_BASE, 0x1000));
    CHECK_EQ_U32((DWORD)g_protects[1].size, 0x1000u);
    CHECK_EQ_U32(g_protects[1].protect, SND_PAGE_EXECUTE_READ);

    CHECK_EQ_PTR(g_protects[2].addr, SND_PTR_ADD(FAKE_BASE, 0x2000));
    CHECK_EQ_U32((DWORD)g_protects[2].size, 0x1000u);
    CHECK_EQ_U32(g_protects[2].protect, SND_PAGE_READONLY);

    CHECK_EQ_PTR(g_protects[3].addr, SND_PTR_ADD(FAKE_BASE, 0x3000));
    CHECK_EQ_U32((DWORD)g_protects[3].size, 0x1000u);
    CHECK_EQ_U32(g_protects[3].protect, SND_PAGE_READWRITE);

    CHECK_EQ_PTR(g_last_start_address, SND_PTR_ADD(FAKE_BASE, 0x1000));

    snd_inj_cleanup(&inj);
    snd_ldr_pe_free_mapped_image(&ldr);
}

static void test_prepare_pe_rejects_missing_entry_point(void) {
    unsigned char raw_pe[0xA00];
    build_multi_section_pe(raw_pe, sizeof(raw_pe), 0);

    snd_buffer_t     raw_buf = {.data = raw_pe, .size = sizeof(raw_pe)};
    snd_ldr_pe_ctx_t ldr     = {0};
    snd_inj_ctx_t    inj     = {0};

    fake_reset();
    ldr.raw_source = &raw_buf;
    ldr.mem_api    = &g_fake_mem;
    ldr.mod_api    = &g_fake_mod;

    inj.target_pid = FAKE_PID;
    inj.proc_api   = &g_fake_proc;

    SND_CHECK_STATUS(snd_inj_classic_pe(&ldr, &inj), SND_STATUS_IMAGE_ENTRY_POINT_MISSING);
    CHECK(inj.stage == SND_INJ_STAGE_UNINITIALIZED);
    CHECK(!log_has("alloc_remote"));

    snd_inj_cleanup(&inj);
    snd_ldr_pe_free_mapped_image(&ldr);
}

static void test_prepare_coff_per_section_protections(void) {
    snd_test_coff_t obj = {0};
#if defined(_WIN64)
    WORD machine = SND_IMAGE_FILE_MACHINE_AMD64;
#else
    WORD machine = SND_IMAGE_FILE_MACHINE_I386;
#endif
    CHECK_EQ_INT(snd_test_coff_build(&obj, machine), 0);
    CHECK(snd_test_coff_add_section(&obj, ".text", 0x60000020u) == 1);
    CHECK(snd_test_coff_add_section(&obj, ".data", 0xC0000040u) == 2);
    CHECK(snd_test_coff_add_section(&obj, ".bss", 0xC0000080u) == 3);

    SND_IMAGE_SYMBOL sym = {0};
    memcpy(sym.N.ShortName, "go", 2);
    sym.SectionNumber = 1;
    CHECK(snd_test_coff_add_symbol(&obj, &sym) == 0);

    memset(&sym, 0, sizeof(sym));
    memcpy(sym.N.ShortName, "bss_var", 7);
    sym.SectionNumber = 0;
    sym.Value         = 16;
    CHECK(snd_test_coff_add_symbol(&obj, &sym) == 1);

    DWORD imp_off = snd_test_coff_add_string(&obj, "KERNEL32$Sleep");
    CHECK(imp_off != 0);
    memset(&sym, 0, sizeof(sym));
    sym.N.Name.Short  = 0;
    sym.N.Name.Long   = imp_off;
    sym.SectionNumber = 0;
    CHECK(snd_test_coff_add_symbol(&obj, &sym) == 2);

    CHECK_EQ_INT(snd_test_coff_finalize(&obj), 0);

    PSND_IMAGE_SECTION_HEADER secs =
        (PSND_IMAGE_SECTION_HEADER)(snd_test_coff_data(&obj) + sizeof(SND_IMAGE_FILE_HEADER));
    secs[0].Misc.VirtualSize = 16;
    secs[1].Misc.VirtualSize = 16;
    secs[2].SizeOfRawData    = 16;
    secs[2].PointerToRawData = 0;

    snd_buffer_t       raw_buf = snd_test_coff_as_buffer(&obj);
    snd_ldr_coff_ctx_t ldr     = {0};
    snd_inj_ctx_t      inj     = {0};
    char               args[12] = "bof_arg_dat";

    fake_reset();
    ldr.raw_source = &raw_buf;
    ldr.mem_api    = &g_fake_mem;
    ldr.mod_api    = &g_fake_mod;

    inj.target_pid = FAKE_PID;
    inj.proc_api   = &g_fake_proc;

    SND_CHECK_SUCCEEDED(snd_inj_classic_coff(&ldr, &inj, "go", args, (int)sizeof(args)));
    CHECK_EQ_U32(g_last_local_alloc_protect, SND_PAGE_READWRITE);
    CHECK_EQ_PTR(inj.remote_arg, SND_PTR_ADD(FAKE_BASE, ldr.target.allocated_size));
    CHECK_EQ_PTR(g_last_start_address, FAKE_BASE);

#if defined(_WIN64)
    CHECK_EQ_INT(g_protect_count, 6);
    CHECK_EQ_PTR(g_protects[4].addr, SND_PTR_ADD(FAKE_BASE, 0x4000));
    CHECK_EQ_U32((DWORD)g_protects[4].size, 0x1000u);
    CHECK_EQ_U32(g_protects[4].protect, SND_PAGE_EXECUTE_READ);
    CHECK_EQ_PTR(g_protects[5].addr, SND_PTR_ADD(FAKE_BASE, 0x5000));
    CHECK_EQ_U32((DWORD)g_protects[5].size, (DWORD)sizeof(args));
    CHECK_EQ_U32(g_protects[5].protect, SND_PAGE_READWRITE);
#else
    CHECK_EQ_INT(g_protect_count, 5);
    CHECK_EQ_PTR(g_protects[4].addr, SND_PTR_ADD(FAKE_BASE, 0x4000));
    CHECK_EQ_U32((DWORD)g_protects[4].size, (DWORD)sizeof(args));
    CHECK_EQ_U32(g_protects[4].protect, SND_PAGE_READWRITE);
#endif

    CHECK_EQ_U32(g_protects[0].protect, SND_PAGE_EXECUTE_READ);
    CHECK_EQ_U32(g_protects[1].protect, SND_PAGE_READWRITE);
    CHECK_EQ_U32(g_protects[2].protect, SND_PAGE_READWRITE);
    CHECK_EQ_U32(g_protects[3].protect, SND_PAGE_READWRITE);

    for (int i = 0; i < g_protect_count; i++) {
        SIZE_T off = SND_PTR_DIFF(g_protects[i].addr, FAKE_BASE);
        CHECK(!SND_RANGE_EXCEEDS(off, g_protects[i].size, g_last_alloc_size));
    }

    snd_inj_cleanup(&inj);
    snd_ldr_coff_free_mapped_image(&ldr);
    snd_test_coff_free(&obj);
}

static void test_prepare_coff_rejects_invalid_entry_section(void) {
    snd_test_coff_t obj = {0};
#if defined(_WIN64)
    WORD machine = SND_IMAGE_FILE_MACHINE_AMD64;
#else
    WORD machine = SND_IMAGE_FILE_MACHINE_I386;
#endif
    CHECK_EQ_INT(snd_test_coff_build(&obj, machine), 0);
    CHECK(snd_test_coff_add_section(&obj, ".text", 0x60000020u) == 1);

    SND_IMAGE_SYMBOL sym = {0};
    memcpy(sym.N.ShortName, "go", 2);
    sym.SectionNumber = -1; /* absolute symbol: not inside any mapped section */
    CHECK(snd_test_coff_add_symbol(&obj, &sym) == 0);
    CHECK_EQ_INT(snd_test_coff_finalize(&obj), 0);

    PSND_IMAGE_SECTION_HEADER secs =
        (PSND_IMAGE_SECTION_HEADER)(snd_test_coff_data(&obj) + sizeof(SND_IMAGE_FILE_HEADER));
    secs[0].Misc.VirtualSize = 16;

    snd_buffer_t       raw_buf = snd_test_coff_as_buffer(&obj);
    snd_ldr_coff_ctx_t ldr     = {0};
    snd_inj_ctx_t      inj     = {0};

    fake_reset();
    ldr.raw_source = &raw_buf;
    ldr.mem_api    = &g_fake_mem;
    ldr.mod_api    = &g_fake_mod;

    inj.target_pid = FAKE_PID;
    inj.proc_api   = &g_fake_proc;

    SND_CHECK_STATUS(snd_inj_classic_coff(&ldr, &inj, "go", NULL, 0), SND_STATUS_COFF_LOADER_ENTRY_POINT_INVALID);
    CHECK(inj.stage == SND_INJ_STAGE_UNINITIALIZED);
    CHECK(log_has("free_remote"));

    snd_inj_cleanup(&inj);
    snd_ldr_coff_free_mapped_image(&ldr);
    snd_test_coff_free(&obj);
}

static void test_set_protections_with_plan_bounds_and_empty_guard(void) {
    snd_inj_ctx_t ctx;
    snd_buffer_t  payload;
    fake_reset();
    init_ctx(&ctx, &payload, &g_fake_proc, &g_fake_thread);

    SND_CHECK_SUCCEEDED(snd_inj_create_suspended_target(&ctx));
    SND_CHECK_SUCCEEDED(snd_inj_alloc_remote(&ctx));
    SND_CHECK_SUCCEEDED(snd_inj_write_payload(&ctx));

    SND_CHECK_STATUS(snd_inj_set_protections_with_plan(&ctx, empty_protect_plan, &ctx), SND_STATUS_CORRUPTED_STAGE);
    CHECK(ctx.stage == SND_INJ_STAGE_PAYLOAD_WRITTEN);

    SND_CHECK_STATUS(snd_inj_set_protections_with_plan(&ctx, oob_protect_plan, &ctx),
                     SND_STATUS_INVALID_PARAMETERS_COMBINATION);
    CHECK(ctx.stage == SND_INJ_STAGE_PAYLOAD_WRITTEN);

    snd_inj_cleanup(&ctx);
}

static void test_coff_local_execute_guards(void) {
    snd_test_coff_t obj = {0};
#if defined(_WIN64)
    WORD machine = SND_IMAGE_FILE_MACHINE_AMD64;
#else
    WORD machine = SND_IMAGE_FILE_MACHINE_I386;
#endif
    CHECK_EQ_INT(snd_test_coff_build(&obj, machine), 0);
    CHECK(snd_test_coff_add_section(&obj, ".text", 0x60000020u) == 1);

    SND_IMAGE_SYMBOL sym = {0};
    memcpy(sym.N.ShortName, "go", 2);
    sym.SectionNumber = 1;
    CHECK(snd_test_coff_add_symbol(&obj, &sym) == 0);
    CHECK_EQ_INT(snd_test_coff_finalize(&obj), 0);

    PSND_IMAGE_SECTION_HEADER secs =
        (PSND_IMAGE_SECTION_HEADER)(snd_test_coff_data(&obj) + sizeof(SND_IMAGE_FILE_HEADER));
    secs[0].Misc.VirtualSize = 16;

    snd_buffer_t       raw_buf = snd_test_coff_as_buffer(&obj);
    snd_ldr_coff_ctx_t ldr     = {0};
    fake_reset();
    ldr.raw_source = &raw_buf;
    ldr.mem_api    = &g_fake_mem;
    ldr.mod_api    = &g_fake_mod;

    SND_CHECK_STATUS(snd_ldr_coff_execute_image(&ldr, NULL, NULL, 0), SND_STATUS_INVALID_STAGE);

    SND_CHECK_SUCCEEDED(snd_ldr_coff_load(&ldr));

    SIZE_T ep_offset = 123;
    SND_CHECK_SUCCEEDED(snd_ldr_coff_resolve_entry_offset(&ldr, NULL, &ep_offset));
    CHECK_EQ_U32((DWORD)ep_offset, 0u);

    LPVOID saved_exec         = ldr.target.execution_base;
    ldr.target.execution_base = FAKE_BASE;
    SND_CHECK_STATUS(snd_ldr_coff_execute_image(&ldr, NULL, NULL, 0), SND_STATUS_COFF_LOADER_LOCAL_EXECUTION_BLOCKED);
    ldr.target.execution_base = saved_exec;

    snd_ldr_coff_free_mapped_image(&ldr);
    snd_test_coff_free(&obj);
}

void snd_test_register_inject_chains(void) {
    snd_test_register("inject chains: classic sequence", test_classic_shell_sequence);
    snd_test_register("inject chains: classic stage guard", test_classic_shell_stage_guard);
    snd_test_register("inject chains: classic error propagation", test_classic_shell_error_propagation);
    snd_test_register("inject chains: classic null table", test_classic_null_table);
    snd_test_register("inject common: staging sequence", test_common_staging_sequence);
    snd_test_register("inject chains: PE per-section protections", test_prepare_pe_per_section_protections);
    snd_test_register("inject chains: PE rejects missing entry", test_prepare_pe_rejects_missing_entry_point);
    snd_test_register("inject chains: COFF per-section protections", test_prepare_coff_per_section_protections);
    snd_test_register("inject chains: COFF rejects invalid entry", test_prepare_coff_rejects_invalid_entry_section);
    snd_test_register("inject chains: protection plan guards", test_set_protections_with_plan_bounds_and_empty_guard);
    snd_test_register("inject chains: COFF local execute guards", test_coff_local_execute_guards);
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
