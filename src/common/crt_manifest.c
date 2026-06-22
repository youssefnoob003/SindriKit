/**
 * @file crt_manifest.c
 * @brief MSVC CRT Intrinsic Override Manifest
 *
 * In a strictly /NODEFAULTLIB environment, the MSVC compiler still generates
 * implicit calls to intrinsic functions (like memcpy and memset) behind the
 * scenes. This typically occurs during struct assignments-by-value or
 * aggressive loop unrolling.
 *
 * This file defines global, standalone implementations of these functions to
 * satisfy the linker and prevent "Unresolved External Symbol" errors,
 * guaranteeing SindriKit remains entirely decoupled from msvcrt.dll and
 * ucrtbase.dll.
 *
 * ARCHITECTURAL WARNING:
 * Do not call these functions explicitly in SindriKit source code. Always use
 * the position-independent `snd_*` primitives defined in `helpers.h` for manual
 * operations.
 */

#include <windows.h>

/*
 * Instruct MSVC to disable internal compiler intrinsics for these specific
 * functions within this translation unit. This forces the compiler to treat our
 * definitions as the absolute global symbols and prevents it from recursively
 * optimizing our byte-loops back into standard CRT calls.
 */
#pragma function(memcpy)
#pragma function(memset)

/**
 * @brief Global compiler-override for implicit memcpy generation.
 * * @param dest Pointer to the destination memory block.
 * @param src Pointer to the source memory block.
 * @param count Number of bytes to copy.
 * @return void* Returns the original destination pointer.
 */
void *memcpy(void *dest, const void *src, size_t count) {
    if (!dest || !src) {
        return dest;
    }

    /*
     * Volatile casting forces strict physical memory writes. It blinds the
     * compiler's optimizer, ensuring it executes the byte-by-byte copy exactly
     * as written without swapping it out for a vectorized CRT intrinsic.
     */
    volatile BYTE       *d = (volatile BYTE *)dest;
    const volatile BYTE *s = (const volatile BYTE *)src;

    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }

    return dest;
}

/**
 * @brief Global compiler-override for implicit memset generation.
 * * @param dest Pointer to the destination memory block.
 * @param c The byte value to set (passed as an int per standard signature).
 * @param count Number of bytes to set.
 * @return void* Returns the original destination pointer.
 */
void *memset(void *dest, int c, size_t count) {
    if (!dest) {
        return dest;
    }

    volatile BYTE *p          = (volatile BYTE *)dest;
    BYTE           fill_value = (BYTE)c;

    for (size_t i = 0; i < count; i++) {
        p[i] = fill_value;
    }

    return dest;
}
