#include <sindri/common/hash.h>
#include <sindri/common/memory.h>
#include <sindri/common/status.h>
#include <sindri/common/string.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/primitives/syscalls.h>
#include <windows.h>

/**
 * @brief Represents a single exported syscall extracted from NTDLL.
 * * By coupling the function's hash with its absolute memory address,
 * we can sort an array of these nodes by address to implicitly
 * recover their Syscall Numbers (SSNs).
 */
typedef struct {
    DWORD hash;
    PVOID address;
} snd_syscall_node_t;

/* * Global cache to prevent re-parsing the Export Address Table (EAT)
 * on every syscall resolution request.
 */
static snd_syscall_node_t g_syscall_table[SND_MAX_SYSCALLS];
static DWORD              g_syscall_count     = 0;
static BOOL               g_table_initialized = FALSE;

/**
 * @brief Sorts the internal syscall cache by memory address in ascending order.
 *
 * Windows maps exported syscalls into NTDLL sequentially. By sorting the
 * extracted functions by their memory addresses, their resulting array
 * index directly corresponds to their underlying Syscall Number (SSN).
 */
static void sort_syscall_table() {
    BOOL swapped;

    for (DWORD i = 0; i < g_syscall_count - 1; i++) {
        swapped = FALSE;

        for (DWORD j = 0; j < g_syscall_count - i - 1; j++) {
            if (g_syscall_table[j].address > g_syscall_table[j + 1].address) {
                // Swap
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

/**
 * @brief Parses the NTDLL Export Address Table (EAT) and populates the cache.
 *
 * This function locates the EAT within the PE headers, iterates through all
 * exported functions, and extracts those prefixed with 'Zw'. It dynamically
 * normalizes the 'Zw' prefix to 'Nt' to ensure the resulting hashes match
 * standard operator inputs, then triggers a sort to finalize the SSN mapping.
 *
 * @param ntdll Base address of the loaded NTDLL module.
 * @return SND_OK on success, or a relevant error status code.
 */
static snd_status_t build_syscall_table(PVOID ntdll) {
    if (g_table_initialized)
        return SND_OK;

    PIMAGE_DOS_HEADER dos            = (PIMAGE_DOS_HEADER)ntdll;
    PIMAGE_NT_HEADERS nt             = (PIMAGE_NT_HEADERS)SND_PTR_ADD(ntdll, dos->e_lfanew);
    DWORD             export_dir_rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

    if (!export_dir_rva)
        return SND_ERR(SND_STATUS_CORRUPTED_STATE);

    PIMAGE_EXPORT_DIRECTORY exp_dir = (PIMAGE_EXPORT_DIRECTORY)SND_PTR_ADD(ntdll, export_dir_rva);
    DWORD                  *names   = (DWORD *)SND_PTR_ADD(ntdll, exp_dir->AddressOfNames);
    DWORD                  *funcs   = (DWORD *)SND_PTR_ADD(ntdll, exp_dir->AddressOfFunctions);
    WORD                   *ords    = (WORD *)SND_PTR_ADD(ntdll, exp_dir->AddressOfNameOrdinals);

    for (DWORD i = 0; i < exp_dir->NumberOfNames; i++) {
        CHAR *func_name = (CHAR *)SND_PTR_ADD(ntdll, names[i]);

        if (func_name[0] == 'Z' && func_name[1] == 'w') {
            if (g_syscall_count >= SND_MAX_SYSCALLS)
                break;

            SIZE_T name_len = snd_strnlen(func_name, SND_MAX_SYS_NAME_LEN);
            CHAR   normalized_name[SND_MAX_SYS_NAME_LEN];

            normalized_name[0] = 'N';
            normalized_name[1] = 't';

            snd_memcpy(&normalized_name[2], &func_name[2], name_len - 1);

            DWORD hash = snd_hash(normalized_name);
            PVOID addr = SND_PTR_ADD(ntdll, funcs[ords[i]]);

            g_syscall_table[g_syscall_count].hash    = hash;
            g_syscall_table[g_syscall_count].address = addr;
            g_syscall_count++;
        }
    }

    sort_syscall_table();
    g_table_initialized = TRUE;

    return SND_OK;
}

snd_status_t snd_syscall_resolve_ssn_sort(PVOID ntdll, DWORD func_hash, snd_syscall_entry_t *entry_out) {
    if (!g_table_initialized) {
        snd_status_t status = build_syscall_table(ntdll);
        if (SND_FAILED(status))
            return status;
    }

    for (DWORD i = 0; i < g_syscall_count; i++) {
        if (g_syscall_table[i].hash == func_hash) {
            entry_out->wSystemCall = (WORD)i;
            return SND_OK;
        }
    }

    return SND_ERR(SND_STATUS_SSN_NOT_FOUND);
}
