#include <windows.h>

DECLSPEC_IMPORT WINUSERAPI int WINAPI USER32$MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);

void go(char *args, int alen) {
    USER32$MessageBoxA(NULL, "Hello from Classic COFF Injection!", "SindriKit", MB_OK | MB_ICONINFORMATION);
}
