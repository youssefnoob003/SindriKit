#ifndef SND_COMMON_DEBUG_H
#define SND_COMMON_DEBUG_H

#include <sindri/common/macros.h>
#include <stdint.h>

/**
 * @def SND_DEBUG
 * @brief Global toggle for debug output.
 */
#ifndef SND_DEBUG
#define SND_DEBUG 0
#endif

/* * Macro block to strip diagnostic error string literals out of the .rdata
 * section entirely when compiling for the production/SILENT tier.
 */
#if SND_DEBUG
#define SND_FALLBACK_STR(debug_str) debug_str
#else
#define SND_FALLBACK_STR(debug_str) ""
#endif

/**
 * @def SND_USE_PRINTF
 * @brief Toggles debug output destination (1 for stdout, 0 for OutputDebugStringA).
 */
#ifndef SND_USE_PRINTF
#define SND_USE_PRINTF 0
#endif

SND_BEGIN_EXTERN_C

#if SND_DEBUG
#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

#if SND_USE_PRINTF

#define SND_FDEBUG_PRINT(stream, fmt, ...)                                                                             \
    do {                                                                                                               \
        FILE *_s = (stream) ? (FILE *)(stream) : stdout;                                                               \
        fprintf(_s, fmt, ##__VA_ARGS__);                                                                               \
        fflush(_s);                                                                                                    \
    } while (0)

#define SND_DEBUG_PRINT(fmt, ...) SND_FDEBUG_PRINT(stdout, fmt, ##__VA_ARGS__)

#else

#define SND_DEBUG_MAX_LEN 1024

static inline void debug_print(const char *fmt, ...) {
    char    buf[SND_DEBUG_MAX_LEN];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    OutputDebugStringA(buf);
}

#define SND_FDEBUG_PRINT(stream, fmt, ...) debug_print(fmt, ##__VA_ARGS__)
#define SND_DEBUG_PRINT(fmt, ...)          debug_print(fmt, ##__VA_ARGS__)

#endif // SND_USE_PRINTF

#else

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
 * @brief Prints a combined hexadecimal and ASCII view of a byte buffer.
 */
void snd_dump_hex(const void *dat, size_t len_dat, uintptr_t base_off);

SND_END_EXTERN_C

#endif // SND_COMMON_DEBUG_H
