#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/parsers/env/peb.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>
#include <windows.h>

snd_status_t snd_syscall_find_gadget_scan(snd_syscall_entry_t *entry) {
    if (entry == NULL || entry->dwHash == 0) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    PVOID        local_ntdll = NULL;
    snd_status_t status      = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &local_ntdll);
    if (SND_FAILED(status) || !local_ntdll) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }

    FARPROC exec_addr = NULL;
    status = snd_pe_get_export_address_hash(local_ntdll, SND_SYS_DLL_SIZE_DEFAULT, entry->dwHash, &exec_addr, NULL);
    if (SND_FAILED(status) || !exec_addr) {
        return SND_ERR(SND_STATUS_SSN_NOT_FOUND);
    }

#if defined(_WIN64)
    BYTE *ptr = (BYTE *)exec_addr;

    // Scan forward a maximum of 32 bytes from the API entry point
    for (int i = 0; i < 32; i++) {
        if (ptr[i] == 0x0F && ptr[i + 1] == 0x05 && ptr[i + 2] == 0xC3) {
            entry->pSyscallAddr = (PVOID)&ptr[i];
            return SND_OK;
        }
    }

    return SND_ERR(SND_STATUS_SSN_NOT_FOUND);
#elif defined(_WIN32)
    BYTE *ptr = (BYTE *)exec_addr;

    // We check if it starts with `mov eax` (0xB8)
    if (ptr[0] == 0xB8) {
        entry->pSyscallAddr = (PVOID)&ptr[5];
        return SND_OK;
    }

    return SND_ERR(SND_STATUS_SSN_NOT_FOUND);
#else
    return SND_ERR(SND_STATUS_ARCH_MISMATCH);
#endif
}
