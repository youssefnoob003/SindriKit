#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define TEST_SIG_ADV_SUCCESS 0x1337C0DE
#define TEST_SIG_ADV_FAILED  0xBAADF00D
#define TEST_SIG_MULTI_OK    0xCA11AB1E
#define TEST_SIG_MULTI_FAIL  0xCA110BAD

/* Track DllMain invocations proves it was called exactly once. */
static volatile DWORD g_attach_count = 0;

/* ── AdvancedExport (unchanged behavior) ───────────────────────────────────
 * Tests kernel32 imports, heap allocation, string manipulation, and
 * dynamic loading of user32.dll.                                           */
__declspec(dllexport) ULONG_PTR AdvancedExport(const char *input_str) {
    if (!input_str)
        return TEST_SIG_ADV_FAILED;

    /* Kernel32: GetSystemInfo */
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    if (sysInfo.dwPageSize == 0)
        return TEST_SIG_ADV_FAILED;

    /* CRT: strlen, malloc, strcpy, strcmp, free */
    size_t len     = strlen(input_str);
    char  *dyn_str = (char *)malloc(len + 1);
    if (!dyn_str)
        return TEST_SIG_ADV_FAILED;

    strcpy(dyn_str, input_str);

    /* Dynamic loading: LoadLibraryA + GetProcAddress */
    HMODULE hUser32 = LoadLibraryA("user32.dll");
    if (!hUser32) {
        free(dyn_str);
        return TEST_SIG_ADV_FAILED;
    }

    FARPROC pMessageBoxA = GetProcAddress(hUser32, "MessageBoxA");
    if (!pMessageBoxA) {
        free(dyn_str);
        return TEST_SIG_ADV_FAILED;
    }

    if (strcmp(dyn_str, "advanced_test") == 0) {
        free(dyn_str);
        return TEST_SIG_ADV_SUCCESS;
    }

    free(dyn_str);
    return TEST_SIG_ADV_FAILED;
}

/* ── VerifyImports ─────────────────────────────────────────────────────────
 * Stress-tests IAT resolution by calling a diverse set of kernel32 APIs
 * across different import entries.  Also checks that DllMain was called
 * exactly once (g_attach_count == 1).                                      */
__declspec(dllexport) ULONG_PTR VerifyImports(void) {
    /* 1. GetCurrentProcessId */
    DWORD pid = GetCurrentProcessId();
    if (pid == 0)
        return TEST_SIG_MULTI_FAIL;

    /* 2. GetCurrentThreadId */
    DWORD tid = GetCurrentThreadId();
    if (tid == 0)
        return TEST_SIG_MULTI_FAIL;

    /* 3. VirtualAlloc / VirtualFree memory management APIs */
    PVOID mem = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem)
        return TEST_SIG_MULTI_FAIL;

    /* Write a pattern and read it back. */
    memset(mem, 0xAA, 4096);
    if (((unsigned char *)mem)[0] != 0xAA) {
        VirtualFree(mem, 0, MEM_RELEASE);
        return TEST_SIG_MULTI_FAIL;
    }
    VirtualFree(mem, 0, MEM_RELEASE);

    /* 4. GetSystemTimeAsFileTime */
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    if (ft.dwLowDateTime == 0 && ft.dwHighDateTime == 0)
        return TEST_SIG_MULTI_FAIL;

    /* 5. DllMain must have been called exactly once. */
    if (g_attach_count != 1)
        return TEST_SIG_MULTI_FAIL;

    return TEST_SIG_MULTI_OK;
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID res) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        g_attach_count++;
        break;
    }
    return TRUE;
}
