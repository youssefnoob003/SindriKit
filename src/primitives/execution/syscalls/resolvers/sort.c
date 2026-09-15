#include <sindri/common/hash.h>
#include <sindri/common/memory.h>
#include <sindri/common/string.h>
#include <sindri/internal/windows/types.h>
#include <sindri/parsers/env/ntdll.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/primitives/status.h>
#include <sindri/primitives/syscalls.h>

typedef struct {
    DWORD hash;
    PVOID address;
} snd_syscall_node_t;

static snd_syscall_node_t g_syscall_table[SND_MAX_SYSCALLS];
static DWORD              g_syscall_count     = 0;
static BOOL               g_table_initialized = FALSE;

static void sort_syscall_table() {
    BOOL swapped;

    for (DWORD i = 0; i < g_syscall_count - 1; i++) {
        swapped = FALSE;

        for (DWORD j = 0; j < g_syscall_count - i - 1; j++) {
            if (g_syscall_table[j].address > g_syscall_table[j + 1].address) {
                snd_syscall_node_t temp = g_syscall_table[j];
                g_syscall_table[j]      = g_syscall_table[j + 1];
                g_syscall_table[j + 1]  = temp;

                swapped = TRUE;
            }
        }

        if (!swapped) {
            break;
        }
    }
}

static BOOL export_enum_callback(const char *func_name, WORD ordinal, PVOID func_addr, PVOID user_ctx) {
    (void)ordinal;
    (void)user_ctx;

    if (!func_name) {
        return TRUE;
    }

    if (func_name[0] == 'Z' && func_name[1] == 'w') {
        if (g_syscall_count >= SND_MAX_SYSCALLS) {
            return FALSE;
        }

        SIZE_T name_len = snd_strnlen(func_name, SND_MAX_SYS_NAME_LEN);
        CHAR   normalized_name[SND_MAX_SYS_NAME_LEN];

        normalized_name[0] = 'N';
        normalized_name[1] = 't';

        snd_memcpy(&normalized_name[2], &func_name[2], name_len - 1);

        DWORD hash = snd_hash(normalized_name);

        g_syscall_table[g_syscall_count].hash    = hash;
        g_syscall_table[g_syscall_count].address = func_addr;
        g_syscall_count++;
    }

    return TRUE;
}

static snd_status_t build_syscall_table() {
    if (g_table_initialized)
        return SND_OK;

    const snd_pe_parser_t *parser = NULL;
    SND_TRY(snd_ntdll_get_clean_parser(&parser));

    SND_TRY(snd_pe_enumerate_exports(parser, export_enum_callback, NULL));

    sort_syscall_table();
    g_table_initialized = TRUE;

    return SND_OK;
}

snd_status_t snd_syscall_resolve_ssn_sort(DWORD func_hash, snd_syscall_entry_t *entry_out) {
    SND_CHECK_NULL(entry_out);

    if (!g_table_initialized) {
        SND_TRY(build_syscall_table());
    }

    for (DWORD i = 0; i < g_syscall_count; i++) {
        if (g_syscall_table[i].hash == func_hash) {
            entry_out->wSystemCall = (WORD)i;
            return SND_OK;
        }
    }

    return SND_ERR_CTX(SND_STATUS_SSN_NOT_FOUND, "Syscall hash 0x%08X not found in sorted NTDLL table", func_hash);
}
