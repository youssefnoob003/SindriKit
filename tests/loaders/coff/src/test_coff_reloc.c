#include <windows.h>

DECLSPEC_IMPORT int MSVCRT$puts(const char *str);
DECLSPEC_IMPORT int MSVCRT$printf(const char *format, ...);

// Uninitialized global data (BSS section)
int global_bss_array[5];

// Initialized global data (DATA section)
int   global_data_val = 1337;
char *global_data_str = "Global string accessed!";

void go(char *args, int alen) {
    global_bss_array[0] = 42;
    global_bss_array[1] = 84;

    if (global_data_val == 1337 && global_bss_array[0] == 42) {
        MSVCRT$puts("COFF Reloc Test: Global DATA and BSS accessed successfully.");
        MSVCRT$printf("COFF Reloc Test: String pointer: %s\n", global_data_str);
    } else {
        MSVCRT$puts("COFF Reloc Test: FAILED");
    }
}
