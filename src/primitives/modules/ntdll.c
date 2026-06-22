#include <sindri/primitives/ntdll.h>

static PVOID g_PristineNtdll = NULL;

void snd_set_ntdll(PVOID ntdll_base) {
    if (ntdll_base == NULL) {
        return;
    }
    g_PristineNtdll = ntdll_base;
}

PVOID snd_get_ntdll(void) {
    return g_PristineNtdll;
}
