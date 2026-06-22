#ifndef SND_COMMON_HELPERS_H
#define SND_COMMON_HELPERS_H

#include <stddef.h>
#include <windows.h>

/**
 * @def SND_DEBUG
 * @brief Global toggle for debug output.
 * Set to 1 to enable logging macros, or 0 to compile them out entirely for
 * Release builds.
 */
#ifndef SND_DEBUG
#define SND_DEBUG 0
#endif

/**
 * @def SND_USE_PRINTF
 * @brief Toggles the debug output destination.
 * If 1, debug macros route to standard stdout via `printf`.
 * If 0, debug macros route to the Windows kernel debugger via
 * `OutputDebugStringA`.
 */
#ifndef SND_USE_PRINTF
#define SND_USE_PRINTF 0
#endif

/**
 * @def SND_BEGIN_EXTERN_C
 * @brief Opens an `extern "C"` linkage block for C++ compatibility.
 */
/**
 * @def SND_END_EXTERN_C
 * @brief Closes an `extern "C"` linkage block.
 */
#ifdef __cplusplus
#define SND_BEGIN_EXTERN_C extern "C" {
#define SND_END_EXTERN_C   }
#else
#define SND_BEGIN_EXTERN_C
#define SND_END_EXTERN_C
#endif

SND_BEGIN_EXTERN_C

/**
 * @def SND_FORCE_INLINE
 * @brief Compiler-agnostic macro to force function inlining.
 * Useful for embedding bounds checks and offset calculations directly into the
 * caller's assembly.
 */
#if defined(_MSC_VER)
#define SND_FORCE_INLINE static __forceinline
#else
#define SND_FORCE_INLINE static inline __attribute__((always_inline))
#endif

#if SND_DEBUG
#include <stdarg.h>
#include <stdio.h>

#if SND_USE_PRINTF
/**
 * @def SND_FDEBUG_PRINT
 * @brief Formats and prints a debug message to a specific file stream (e.g.,
 * stderr, stdout).
 */
#define SND_FDEBUG_PRINT(stream, fmt, ...)                                                                             \
    do {                                                                                                               \
        FILE *_s = (stream) ? (FILE *)(stream) : stdout;                                                               \
        fprintf(_s, fmt, ##__VA_ARGS__);                                                                               \
        fflush(_s);                                                                                                    \
    } while (0)

/**
 * @def SND_DEBUG_PRINT
 * @brief Formats and prints a debug message to standard output.
 */
#define SND_DEBUG_PRINT(fmt, ...) SND_FDEBUG_PRINT(stdout, fmt, ##__VA_ARGS__)

#else
/**
 * @def SND_DEBUG_MAX_LEN
 * @brief Maximum string length for the internal debug formatting buffer.
 */
#define SND_DEBUG_MAX_LEN                  1024

/**
 * @brief Internal variadic wrapper to format strings for the Windows Debugger.
 * @note This is completely stripped out of Release builds.
 */
static inline void snd_debug_print_internal(const char *fmt, ...) {
    char    buf[SND_DEBUG_MAX_LEN];
    va_list args;

    va_start(args, fmt);
    // Safely format the string into our local buffer (requires CRT in debug mode)
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    // Send the formatted string to the Windows kernel debug buffer
    OutputDebugStringA(buf);
}

// Map both macros to the new OutputDebugString wrapper
#define SND_FDEBUG_PRINT(stream, fmt, ...) snd_debug_print_internal(fmt, ##__VA_ARGS__)

#define SND_DEBUG_PRINT(fmt, ...) snd_debug_print_internal(fmt, ##__VA_ARGS__)

#endif // SND_USE_PRINTF

#else
// When SND_DEBUG is 0, these macros compile to empty no-ops to strip strings
// from the binary.
#define SND_FDEBUG_PRINT(stream, fmt, ...)                                                                             \
    do {                                                                                                               \
        (void)(stream);                                                                                                \
    } while (0)

#define SND_DEBUG_PRINT(fmt, ...)                                                                                      \
    do {                                                                                                               \
        (void)0;                                                                                                       \
    } while (0)
#endif // SND_DEBUG

/**
 * @def SND_PTR_ADD
 * @brief Safely adds a byte offset to a base pointer by casting to a BYTE
 * pointer first.
 */
#define SND_PTR_ADD(base, offset) ((BYTE *)(base) + (offset))

/**
 * @brief Prints a combined hexadecimal and ASCII view of a byte buffer.
 * @param dat Pointer to the first byte to print.
 * @param len_dat Number of bytes to dump from @p dat.
 * @param base_off Base address used to compute printed relative offsets.
 *
 * @note Output is gated by `SND_DEBUG` through debug print macros.
 */
void snd_dump_hex(const void *dat, size_t len_dat, ULONG_PTR base_off);

/**
 * @brief Checks if a given size and offset are within the bounds of a total
 * size.
 * @param total_size The total size of the buffer.
 * @param offset The offset from the start.
 * @param size The size of the region to check.
 * @return TRUE if within bounds, FALSE otherwise.
 */
SND_FORCE_INLINE BOOL snd_bounds_check(SIZE_T total_size, SIZE_T offset, SIZE_T size) {
    if (offset > total_size)
        return FALSE;
    if (size > total_size - offset)
        return FALSE;
    return TRUE;
}

/**
 * @brief Checks if a pointer and size are within the bounds of a base pointer
 * and total size.
 * @param base The base pointer.
 * @param total_size The total size of the base buffer.
 * @param ptr The target pointer to check.
 * @param size The size of the region at the target pointer.
 * @return TRUE if within bounds, FALSE otherwise.
 */
SND_FORCE_INLINE BOOL snd_ptr_bounds_check(const void *base, SIZE_T total_size, const void *ptr, SIZE_T size) {
    if (base == NULL || ptr == NULL)
        return FALSE;
    ULONG_PTR ubase   = (ULONG_PTR)base;
    ULONG_PTR utarget = (ULONG_PTR)ptr;
    if (utarget < ubase)
        return FALSE;
    SIZE_T offset = (SIZE_T)(utarget - ubase);
    return snd_bounds_check(total_size, offset, size);
}

/**
 * @brief Bounded, CRT-independent memory zeroing routine.
 * Replaces memset.
 */
SND_FORCE_INLINE void snd_memzero(void *dest, size_t size) {
    if (!dest)
        return;
    volatile BYTE *p = (volatile BYTE *)dest;
    for (size_t i = 0; i < size; i++) {
        p[i] = 0;
    }
}

/**
 * @brief Raw byte copy routine.
 * Replaces memcpy.
 */
SND_FORCE_INLINE void snd_memcpy(void *dest, const void *src, size_t count) {
    if (!dest || !src)
        return;
    BYTE       *d = (BYTE *)dest;
    const BYTE *s = (const BYTE *)src;
    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
}

/**
 * @brief Bounded, CRT-independent string length evaluation.
 * Replaces strnlen.
 */
SND_FORCE_INLINE size_t snd_strnlen(const char *str, size_t max_len) {
    if (!str)
        return 0;
    size_t len = 0;
    while (len < max_len && str[len] != '\0') {
        len++;
    }
    return len;
}

/**
 * @brief Bounded, truncation-safe string copying.
 * Replaces strncpy / strncpy_s.
 */
SND_FORCE_INLINE void snd_strncpy(char *dest, size_t dest_size, const char *src, size_t max_src_len) {
    if (!dest || dest_size == 0 || !src)
        return;

    size_t i = 0;
    while (i < (dest_size - 1) && i < max_src_len && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/**
 * @brief Bounded, CRT-independent string concatenation.
 * Replaces strcat / strcat_s.
 */
static inline void snd_strncat(char *dest, SIZE_T dest_size, const char *src, SIZE_T max_src_len) {
    if (!dest || dest_size == 0 || !src)
        return;

    SIZE_T dest_len = 0;
    while (dest_len < dest_size && dest[dest_len] != '\0') {
        dest_len++;
    }

    if (dest_len >= dest_size - 1) {
        dest[dest_size - 1] = '\0';
        return;
    }

    SIZE_T i = 0;
    while (i < max_src_len && src[i] != '\0' && (dest_len + i) < (dest_size - 1)) {
        dest[dest_len + i] = src[i];
        i++;
    }

    dest[dest_len + i] = '\0';
}

/**
 * @brief Bounded, CRT-independent character search.
 * Replaces strchr.
 */
static inline const char *snd_strnchr(const char *str, char c, SIZE_T max_len) {
    if (!str)
        return NULL;

    for (SIZE_T i = 0; i < max_len; i++) {
        if (str[i] == c) {
            return &str[i];
        }
        if (str[i] == '\0') {
            break;
        }
    }

    return NULL;
}

/**
 * @brief Bounded, CRT-independent string comparison.
 * Replaces strcmp / strncmp.
 */
static inline int snd_strncmp(const char *s1, const char *s2, SIZE_T max_len) {
    if (!s1 || !s2)
        return (s1 == s2) ? 0 : ((s1 < s2) ? -1 : 1);

    for (SIZE_T i = 0; i < max_len; i++) {
        if (s1[i] != s2[i]) {
            return (int)((unsigned char)s1[i] - (unsigned char)s2[i]);
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }

    return 0;
}

SND_END_EXTERN_C

#endif // SND_COMMON_HELPERS_H
