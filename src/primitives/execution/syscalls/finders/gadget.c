#include <sindri/common/macros.h>
#include <sindri/common/opcodes.h>
#include <sindri/internal/windows/types.h>
#include <sindri/parsers/env/ntdll.h>
#include <sindri/primitives/status.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>

snd_status_t snd_syscall_find_gadget_scan(snd_syscall_entry_t *entry) {
    SND_CHECK_NULL(entry, entry->dwHash);

    FARPROC exec_addr = NULL;
    SND_TRY(snd_ntdll_get_active_export(entry->dwHash, &exec_addr));

#if defined(_WIN64)
    BYTE *ptr = (BYTE *)exec_addr;

    for (int i = 0; i < 32; i++) {
        if (ptr[i] == SND_OPCODE_SYSCALL_0 && ptr[i + 1] == SND_OPCODE_SYSCALL_1 && ptr[i + 2] == SND_OPCODE_RET) {
            entry->pSyscallAddr = (PVOID)&ptr[i];
            return SND_OK;
        }
    }

    return SND_ERR_CTX(SND_STATUS_GADGET_NOT_FOUND, "No syscall; ret gadget found in 32-byte window for hash 0x%08X",
                       entry->dwHash);
#elif defined(_WIN32)
    BYTE *ptr = (BYTE *)exec_addr;

    if (ptr[0] == SND_OPCODE_MOV_EAX_IMM32) {
        entry->pSyscallAddr = (PVOID)&ptr[5];
        return SND_OK;
    }

    return SND_ERR_CTX(SND_STATUS_GADGET_NOT_FOUND, "No x86 sysenter stub pattern found for hash 0x%08X",
                       entry->dwHash);
#else
    return SND_ERR_CTX(SND_STATUS_ARCH_MISMATCH, "Gadget scan unsupported on target architecture");
#endif
}
