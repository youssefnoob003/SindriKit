#include <windows.h>

#define SND_MARKER_NAME "snd_inject_probe.tmp"

DECLSPEC_IMPORT DWORD WINAPI  KERNEL32$GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                                                   LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                                   DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                                                   HANDLE hTemplateFile);
DECLSPEC_IMPORT BOOL WINAPI   KERNEL32$WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                                                 LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
DECLSPEC_IMPORT BOOL WINAPI   KERNEL32$CloseHandle(HANDLE hObject);

static void snd_write_marker(void) {
    static const char content[] = "SND_INJECT_OK";
    char              exe[MAX_PATH];
    DWORD             written = 0;
    HANDLE            h;

    DWORD n = KERNEL32$GetModuleFileNameA(NULL, exe, sizeof(exe));
    if (n == 0 || n >= sizeof(exe)) {
        return;
    }

    DWORD dir_len = n;
    while (dir_len > 0 && exe[dir_len - 1] != '\\' && exe[dir_len - 1] != '/') {
        dir_len--;
    }
    if (dir_len == 0 || dir_len + sizeof(SND_MARKER_NAME) > sizeof(exe)) {
        return;
    }

    const char *name = SND_MARKER_NAME;
    for (DWORD i = 0; i < sizeof(SND_MARKER_NAME); i++) {
        exe[dir_len + i] = name[i];
    }

    h = KERNEL32$CreateFileA(exe, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        KERNEL32$WriteFile(h, content, (DWORD)(sizeof(content) - 1), &written, NULL);
        KERNEL32$CloseHandle(h);
    }
}

void go(char *args, int alen) {
    (void)args;
    (void)alen;
    snd_write_marker();
}