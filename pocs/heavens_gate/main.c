#include <sindri.h>
#include <stdio.h>
#include <windows.h>

int main(void) {
    printf("[*] SindriKit Heaven's Gate PoC\n");

#if defined(_WIN64)
    printf("[-] This PoC is compiled as 64-bit native. Heaven's Gate is a 32-bit "
           "WOW64 technique.\n");
    printf("[-] Please compile the project for x86 (32-bit) to test Heaven's "
           "Gate.\n");
    return SND_STATUS_ARCH_MISMATCH;
#elif defined(_WIN32)
    if (!snd_is_wow64()) {
        printf("[-] Not running in WOW64 (either 32-bit OS or native 64-bit "
               "process). Heaven's Gate requires WOW64.\n");
        return SND_STATUS_ARCH_MISMATCH;
    }

    printf("[+] WOW64 environment detected (using native PEB parsing).\n");

    /*
     * 64-bit Shellcode to execute.
     *
     * Assembly (x64):
     *   mov rax, 0x1122334455667788
     *   ret
     *
     * Bytes: 48 B8 88 77 66 55 44 33 22 11 C3
     */
    unsigned char shellcode64[] = {
        0x48, 0xB8, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, // mov rax, 0x1122334455667788
        0xC3                                                        // ret
    };

    // Allocate executable memory for the shellcode
    PVOID pExec = VirtualAlloc(NULL, sizeof(shellcode64), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!pExec) {
        printf("[-] Failed to allocate executable memory.\n");
        return SND_ERROR_GENERIC;
    }

    memcpy(pExec, shellcode64, sizeof(shellcode64));

    printf("[*] Executing 64-bit shellcode at 0x%p via Heaven's Gate...\n", pExec);

    UINT64       result = 0;
    snd_status_t status = snd_hg_execute_64((UINT64)(ULONG_PTR)pExec, 0, NULL, &result);

    if (SND_SUCCEEDED(status)) {
        printf("[+] Heaven's Gate execution successful!\n");
        printf("[+] 64-bit code returned: 0x%llx (Expected: 0x1122334455667788)\n", result);
    } else {
        printf("[-] Heaven's Gate execution failed with status code: 0x%08X\n", status.code);
    }

    VirtualFree(pExec, 0, MEM_RELEASE);
    return SND_SUCCESS;
#else
#error "Unsupported architecture: SindriKit requires _WIN32 or _WIN64"
#endif
}
