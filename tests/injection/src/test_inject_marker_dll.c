#include <windows.h>

#define SND_MARKER_NAME "snd_inject_probe.tmp"

/* External entry point (must not be static, or /ENTRY cannot resolve it). */
void snd_marker_entry(void);

/* Absolute initializer -> DIR64 base relocation. `volatile` keeps the compiler
 * from constant-folding the reference away (which would strip the anchor). */
static void (*volatile g_snd_reloc_anchor)(void) = snd_marker_entry;

static BOOL snd_write_marker(void) {
    static const char content[] = "SND_INJECT_OK";
    char              exe[MAX_PATH];
    DWORD             written = 0;
    HANDLE            h;

    DWORD n = GetModuleFileNameA(NULL, exe, sizeof(exe));
    if (n == 0 || n >= sizeof(exe)) {
        return FALSE;
    }

    /* Find the directory prefix (through the last separator) and overwrite the
     * file name with the marker name in place. */
    DWORD dir_len = n;
    while (dir_len > 0 && exe[dir_len - 1] != '\\' && exe[dir_len - 1] != '/') {
        dir_len--;
    }
    if (dir_len == 0 || dir_len + sizeof(SND_MARKER_NAME) > sizeof(exe)) {
        return FALSE;
    }

    const char *name = SND_MARKER_NAME;
    for (DWORD i = 0; i < sizeof(SND_MARKER_NAME); i++) {
        exe[dir_len + i] = name[i];
    }

    h = CreateFileA(exe, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    BOOL ok = WriteFile(h, content, (DWORD)(sizeof(content) - 1), &written, NULL);
    CloseHandle(h);
    return ok && written == sizeof(content) - 1;
}

/* Injection entry point: ignores arguments (none are meaningfully provided). */
void snd_marker_entry(void) {
    if (g_snd_reloc_anchor == NULL) {
        return; /* unreachable: keeps the relocation anchor referenced */
    }
    snd_write_marker();
}