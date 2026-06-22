#include <windows.h>

/* ── TLS Callback Test DLL ─────────────────────────────────────────────────
 * Registers a TLS callback via the CRT initializer list.  The reflective
 * loader must parse the IMAGE_TLS_DIRECTORY and fire the callbacks
 * before calling DllMain.  VerifyTLS checks whether the callback ran.     */

#define TEST_SIG_TLS_OK   0x71500C01
#define TEST_SIG_TLS_FAIL 0x715FA110

/* Sentinel written by the TLS callback. */
static volatile ULONG_PTR g_tls_magic = 0;
#define TLS_SENTINEL 0x7153CA11

/* ── TLS callback registration (MSVC) ─────────────────────────────────────
 * Force the linker to emit an IMAGE_TLS_DIRECTORY so the loader has
 * something to parse.                                                      */
#ifdef _M_IX86
#pragma comment(linker, "/INCLUDE:__tls_used")
#else
#pragma comment(linker, "/INCLUDE:_tls_used")
#endif

static void NTAPI snd_tls_callback(PVOID DllHandle, DWORD Reason, PVOID Reserved) {
    if (Reason == DLL_PROCESS_ATTACH) {
        g_tls_magic = TLS_SENTINEL;
    }
}

/* Place our callback pointer in the .CRT$XLB section so it appears in
 * the TLS callback array between .CRT$XLA and .CRT$XLZ.                   */
#pragma section(".CRT$XLB", read)
__declspec(allocate(".CRT$XLB")) PIMAGE_TLS_CALLBACK p_tls_callback = snd_tls_callback;

/* ── Export ────────────────────────────────────────────────────────────────*/

__declspec(dllexport) ULONG_PTR VerifyTLS(void) {
    if (g_tls_magic == TLS_SENTINEL)
        return TEST_SIG_TLS_OK;
    return TEST_SIG_TLS_FAIL;
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID res) {
    return TRUE;
}
