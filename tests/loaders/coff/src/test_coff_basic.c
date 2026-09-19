#include <windows.h>

DECLSPEC_IMPORT int MSVCRT$puts(const char *str);

void go(char *args, int alen) {
    (void)args;
    (void)alen;
    MSVCRT$puts("COFF Basic Test: Execution successful.");
}
