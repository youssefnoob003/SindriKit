#include <sindri/common/status.h>
#include <sindri/primitives/syscalls.h>
#include <windows.h>

#define SND_MAX_INTERNAL_STRATEGIES 4

static PVOID g_syscall_ntdll_target = NULL;

static snd_syscall_resolver_t g_strategy_chain[SND_MAX_INTERNAL_STRATEGIES] = {SND_SYSCALL_RESOLVER_DEFAULT};
#if SND_USE_DEFAULTS
static int g_strategy_count = 1;
#else
static int g_strategy_count = 0;
#endif

snd_syscall_invoker_t       g_syscall_invoker       = SND_SYSCALL_INVOKER_DEFAULT;
snd_syscall_gadget_finder_t g_syscall_gadget_finder = SND_SYSCALL_GADGET_FINDER_DEFAULT;

void snd_syscall_set_ntdll(PVOID ntdll_base) {
    if (!ntdll_base)
        return;
    g_syscall_ntdll_target = ntdll_base;
}

void snd_syscall_set_invoker(snd_syscall_invoker_t invoker) {
    if (!invoker)
        return;
    g_syscall_invoker = invoker;
}

void snd_syscall_set_gadget_finder(snd_syscall_gadget_finder_t finder) {
    if (!finder)
        return;
    g_syscall_gadget_finder = finder;
}

void snd_syscall_set_resolver(snd_syscall_resolver_t resolver) {
    if (!resolver)
        return;
    g_strategy_chain[0] = resolver;
    g_strategy_count    = 1;
}

snd_status_t snd_syscall_add_resolver(snd_syscall_resolver_t resolver) {
    if (!resolver) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }
    if (g_strategy_count < SND_MAX_INTERNAL_STRATEGIES) {
        g_strategy_chain[g_strategy_count++] = resolver;
        return SND_OK;
    }
    return SND_ERR(SND_STATUS_PIPELINE_EXHAUSTED);
}

snd_status_t snd_syscall_resolve(DWORD func_hash, snd_syscall_entry_t *entry_out) {
    if (!entry_out) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    if (!g_syscall_ntdll_target) {
        return SND_ERR_CTX(SND_STATUS_NTDLL_NOT_INITIALIZED, "NTDLL base is NULL");
    }

    if (g_strategy_count == 0) {
        return SND_ERR_CTX(SND_STATUS_RESOLVER_NOT_INITIALIZED, "No syscall strategies set");
    }

    for (int i = 0; i < g_strategy_count; i++) {
        if (!g_strategy_chain[i])
            continue;

        snd_status_t status = g_strategy_chain[i](g_syscall_ntdll_target, func_hash, entry_out);

        if (SND_SUCCEEDED(status)) {
            if (g_syscall_gadget_finder != NULL) {
                status = g_syscall_gadget_finder(entry_out);
            }
            return status;
        }
    }

    return SND_ERR_CTX(SND_STATUS_SSN_NOT_FOUND, "func_hash=0x%08X", func_hash);
}
