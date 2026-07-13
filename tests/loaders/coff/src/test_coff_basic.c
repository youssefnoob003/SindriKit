#include <windows.h>

DECLSPEC_IMPORT WINBASEAPI int WINAPI USER32$MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
DECLSPEC_IMPORT int                   MSVCRT$puts(const char *str);

void go(char *args, int alen) {
    MSVCRT$puts("COFF Basic Test: Execution successful.");
}
