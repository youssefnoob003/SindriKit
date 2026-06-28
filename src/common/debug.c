#if SND_DEBUG
#include <ctype.h>
#endif
#include <sindri/common/debug.h>
#include <stdint.h>

void snd_dump_hex(const void *dat, size_t len_dat, uintptr_t base_off) {
#if SND_DEBUG
    uintptr_t   real_offset = (uintptr_t)dat - base_off;
    const BYTE *bytes       = (const BYTE *)dat;

    for (size_t i = 0; i < len_dat; i += 0x10) {
        SND_DEBUG_PRINT("0x%08zX\t", (size_t)(real_offset + i));

        for (size_t j = 0; j < 16; j++) {
            if (i + j < len_dat) {
                SND_DEBUG_PRINT("%02x ", bytes[i + j]);
            } else {
                SND_DEBUG_PRINT("   ");
            }
        }
        SND_DEBUG_PRINT("  ");

        for (size_t j = 0; j < 16; j++) {
            if (i + j < len_dat) {
                BYTE c = bytes[i + j];
                SND_DEBUG_PRINT("%c", isprint(c) ? c : '.');
            } else {
                SND_DEBUG_PRINT(" ");
            }
        }
        SND_DEBUG_PRINT("\n");
    }
#else
    (void)dat;
    (void)len_dat;
    (void)base_off;
#endif
}
