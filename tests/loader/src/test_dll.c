#include <string.h>
#include <windows.h>

#define TEST_SIG_SUCCESS  0xFEEDC0DE
#define TEST_SIG_BAD_ARGS 0xDEADBEEF
#define TEST_SIG_INIT_OK  0xC001D00D
#define TEST_SIG_NO_INIT  0xBAD10001
#define TEST_SIG_NO_IAT   0xBAD10002

/* ── Relocation & DllMain verification ─────────────────────────────────────
 * g_attach_magic lives in .data.  If the loader doesn't apply relocations
 * every reference to it will hit the wrong address.  DllMain sets it to a
 * known sentinel; VerifyInit reads it back.                                 */
static volatile ULONG_PTR g_attach_magic = 0;
#define ATTACH_SENTINEL 0xA77AC4ED

/* ── Exports ───────────────────────────────────────────────────────────────*/

/* Original export validates that the caller passed the exact expected
 * three arguments through the FFI bridge.                                  */
__declspec(dllexport) ULONG_PTR SayHello(const char *s1, const char *s2, ULONG_PTR n3) {
    if (!s1 || !s2)
        return TEST_SIG_BAD_ARGS;

    if (strcmp(s1, "bonjour") == 0 && strcmp(s2, "hello") == 0 && n3 == 12) {
        return TEST_SIG_SUCCESS;
    }

    return TEST_SIG_BAD_ARGS;
}

/* Checks that DllMain(DLL_PROCESS_ATTACH) ran **and** that the global it
 * wrote to is reachable (i.e. relocations were applied correctly).
 * Also calls a kernel32 import to verify the IAT was resolved.             */
__declspec(dllexport) ULONG_PTR VerifyInit(void) {
    /* 1. DllMain must have written the sentinel into the relocated global. */
    if (g_attach_magic != ATTACH_SENTINEL)
        return TEST_SIG_NO_INIT;

    /* 2. Cheap kernel32 call if the IAT wasn't patched this will crash. */
    if (GetCurrentProcessId() == 0)
        return TEST_SIG_NO_IAT;

    return TEST_SIG_INIT_OK;
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID res) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_attach_magic = ATTACH_SENTINEL;
    }
    return TRUE;
}
