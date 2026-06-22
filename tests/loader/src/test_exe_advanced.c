#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* Exit codes */
#define EXIT_OK        0x1337
#define EXIT_ALLOC_BAD 0xBAAD
#define EXIT_CRT_FAIL  0xC210

int main(int argc, char *argv[]) {
    /* ── CRT validation ──────────────────────────────────────────────────────
     * Verify that the C runtime was initialized properly by the CRT entry
     * point.  If any of these fail the loader botched CRT startup.          */

    const char *test_str = "SindriKit";
    char        buf[16];
    memcpy(buf, test_str, strlen(test_str) + 1);
    if (strcmp(buf, "SindriKit") != 0)
        return EXIT_CRT_FAIL;

    /* ── Heap allocation + printf (original behavior) ────────────────────── */

    printf("SindriKit Advanced EXE Test\n");

    char *buffer = (char *)malloc(1024);
    if (buffer) {
        sprintf(buffer, "Successfully allocated and printed! Arg count: %d\n", argc);
        printf("%s", buffer);
        free(buffer);
    } else {
        return EXIT_ALLOC_BAD;
    }

    /* ── Multi-API verification ──────────────────────────────────────────── */

    DWORD pid = GetCurrentProcessId();
    DWORD tid = GetCurrentThreadId();
    printf("[+] CRT OK | PID=%lu TID=%lu\n", (unsigned long)pid, (unsigned long)tid);

    return EXIT_OK;
}
