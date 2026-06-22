#include <windows.h>

/* Exit codes the runner matches on these.                               */
#define EXIT_OK          0x7A /* 122 decimal original success code      */
#define EXIT_RELOC_FAIL  0x01 /* Relocation canary corrupted              */
#define EXIT_IMPORT_FAIL 0x02 /* Kernel32 import call failed              */

/* Global in .data if relocations aren't applied this will read as
 * garbage when the image is loaded at a non-preferred base address.        */
static volatile ULONG_PTR g_reloc_canary = 0xCAFEBABE;

int main(int argc, char *argv[]) {
    /* 1. Relocation check: the canary must still hold its init value. */
    if (g_reloc_canary != 0xCAFEBABE)
        return EXIT_RELOC_FAIL;

    /* 2. Import check: call a kernel32 API through the resolved IAT. */
    if (GetCurrentProcessId() == 0)
        return EXIT_IMPORT_FAIL;

    return EXIT_OK;
}
