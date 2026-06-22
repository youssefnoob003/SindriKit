#include <sindri/common/status.h>
#include <sindri/primitives/peb.h>
#include <sindri/primitives/syscalls.h>
#include <windows.h>

#define SND_MAX_INTERNAL_STRATEGIES 4

// State management for the resolution pipeline
static snd_syscall_resolver_t g_strategy_chain[SND_MAX_INTERNAL_STRATEGIES] = {0};
static int                    g_strategy_count                              = 0;
static PVOID g_syscall_ntdll_target = NULL;

void snd_set_ntdll(PVOID ntdll_base) {
    if (ntdll_base == NULL) {
        return;
    }
    g_syscall_ntdll_target = ntdll_base;
}

PVOID snd_get_ntdll(void) {
    return g_syscall_ntdll_target;
}

void snd_set_syscall_strategy(snd_syscall_resolver_t resolver) {
    if (!resolver)
        return;
    g_strategy_chain[0] = resolver;
    g_strategy_count    = 1;
}

snd_status_t snd_add_syscall_strategy(snd_syscall_resolver_t resolver) {
    if (!resolver) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }
    if (g_strategy_count < SND_MAX_INTERNAL_STRATEGIES) {
        g_strategy_chain[g_strategy_count++] = resolver;
        return SND_OK;
    }
    return SND_ERR(SND_STATUS_SSN_BUFFER_TOO_SMALL);
}

snd_status_t snd_resolve_syscall(DWORD func_hash, snd_syscall_entry_t *entry_out) {
    if (!entry_out) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }
    PVOID ntdll = snd_get_ntdll();
    if (!ntdll) {
        return SND_ERR_CTX(SND_STATUS_NOT_INITIALIZED, "NTDLL base is NULL");
    }

    if (g_strategy_count == 0) {
        return SND_ERR_CTX(SND_STATUS_NOT_INITIALIZED, "No syscall strategies set");
    }

    // Iterate through the pipeline of provided resolvers
    for (int i = 0; i < g_strategy_count; i++) {
        if (!g_strategy_chain[i])
            continue;

        snd_status_t status = g_strategy_chain[i](ntdll, func_hash, entry_out);

        if (status.code == SND_SUCCESS) {
            return status;
        }
    }

    return SND_ERR_CTX(SND_STATUS_SSN_NOT_FOUND, "func_hash=0x%08X", func_hash);
}
