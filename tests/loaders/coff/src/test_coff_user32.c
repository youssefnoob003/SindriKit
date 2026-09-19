#include <windows.h>

DECLSPEC_IMPORT WINUSERAPI HWND WINAPI USER32$GetDesktopWindow(void);
DECLSPEC_IMPORT int                    MSVCRT$puts(const char *str);

void go(char *args, int alen) {
    (void)args;
    (void)alen;
    if (USER32$GetDesktopWindow() != NULL) {
        MSVCRT$puts("COFF USER32 Test: GetDesktopWindow resolved.");
    } else {
        MSVCRT$puts("COFF USER32 Test: GetDesktopWindow returned NULL.");
    }
}
