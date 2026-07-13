#include <windows.h>

DECLSPEC_IMPORT int MSVCRT$puts(const char *str);
DECLSPEC_IMPORT int MSVCRT$printf(const char *format, ...);

void go(char *args, int alen) {
    if (args == NULL || alen <= 0) {
        MSVCRT$puts("COFF Args Test: No arguments provided.");
        return;
    }

    MSVCRT$printf("COFF Args Test: Received %d bytes of arguments.\n", alen);

    // Unpack arguments according to BOF specification (assuming strings for simplicity)
    // First 4 bytes are usually the size of the total payload, but we'll just print it as a buffer
    MSVCRT$puts("COFF Args Test: Execution successful.");
}
