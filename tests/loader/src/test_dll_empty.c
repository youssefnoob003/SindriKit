#include <windows.h>

// A completely empty DLL.
// No exports, no standard imports (other than what the compiler forces for
// DllMain). Used to test if the loader gracefully handles missing directories
// (like the Export Directory).

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID res) {
    return TRUE;
}
