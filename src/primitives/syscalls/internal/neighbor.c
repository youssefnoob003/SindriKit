#include "neighbor.h"

#include <sindri/common/helpers.h>

snd_status_t snd_neighbor_search_ssn(BYTE *p, PVOID ntdll, SIZE_T image_size, WORD *ssn_out) {
#if defined(_WIN64)
    for (WORD idx = 1; idx <= 500; idx++) {
        // Check up bounds
        BYTE *up_ptr = p + (idx * 32);
        if (snd_ptr_bounds_check(ntdll, image_size, up_ptr, 8)) {
            if (*((DWORD *)up_ptr) == 0xB8D18B4C) { /* 4C 8B D1 B8 */
                *ssn_out = *(WORD *)(up_ptr + 4) - idx;
                return SND_OK;
            }
        }
        // Check down bounds
        BYTE *down_ptr = p - (idx * 32);
        if (snd_ptr_bounds_check(ntdll, image_size, down_ptr, 8)) {
            if (*((DWORD *)down_ptr) == 0xB8D18B4C) { /* 4C 8B D1 B8 */
                *ssn_out = *(WORD *)(down_ptr + 4) + idx;
                return SND_OK;
            }
        }
    }
#elif defined(_WIN32)
    for (WORD idx = 1; idx <= 500; idx++) {
        // Check up bounds
        BYTE *up_ptr = p + (idx * 16);
        if (snd_ptr_bounds_check(ntdll, image_size, up_ptr, 4)) {
            if (*up_ptr == 0xB8) {
                *ssn_out = *(WORD *)(up_ptr + 1) - idx;
                return SND_OK;
            }
        }
        // Check down bounds
        BYTE *down_ptr = p - (idx * 16);
        if (snd_ptr_bounds_check(ntdll, image_size, down_ptr, 4)) {
            if (*down_ptr == 0xB8) {
                *ssn_out = *(WORD *)(down_ptr + 1) + idx;
                return SND_OK;
            }
        }
    }
#else
#error "Unsupported architecture: SindriKit requires _WIN32 or _WIN64"
#endif
    return SND_ERR(SND_STATUS_SSN_NOT_FOUND);
}
