#include <sindri/common/hash.h>
#include <sindri/common/macros.h>
#include <sindri_hashes.h>
#include <stdint.h>

/**
 * @def SND_HASH_SEED
 * @brief Fallback initialization constant for hashing algorithms.
 * * Under normal build conditions, SND_HASH_SEED is randomly generated
 * by the Python build script and provided via sindri_hashes.h to prevent
 * API hash fingerprinting.
 * * If the header fails to define it (e.g., manual compilation without the
 * build system), we fall back to the standard, ingerprintable algorithm
 * constants to prevent compilation failure.
 */
#ifndef SND_HASH_SEED
#if defined(SND_HASH_FNV1A)
#pragma message("WARNING: Using standard FNV-1a seed. Binary is easily fingerprintable.")
#define SND_HASH_SEED 2166136261U
#else
#pragma message("WARNING: Using standard djb2 seed. Binary is easily fingerprintable.")
#define SND_HASH_SEED 5381U
#endif
#endif

SND_FORCE_INLINE uint32_t snd_hash_djb2_internal(const char *str) {
    if (!str)
        return 0;
    uint32_t hash = SND_HASH_SEED;
    while (*str) {
        hash = hash * 33 + (unsigned char)*str;
        str++;
    }
    return hash;
}

SND_FORCE_INLINE uint32_t snd_hash_fnv1a_internal(const char *str) {
    if (!str)
        return 0;
    uint32_t hash = SND_HASH_SEED;
    while (*str) {
        hash ^= (unsigned char)*str;
        hash *= 16777619U;
        str++;
    }
    return hash;
}

SND_FORCE_INLINE uint32_t snd_hash_djb2_lower_internal(const char *str) {
    if (!str)
        return 0;
    uint32_t hash = SND_HASH_SEED;
    while (*str) {
        char c = *str;
        if (c >= 'A' && c <= 'Z') {
            c += ('a' - 'A');
        }
        hash = hash * 33 + (unsigned char)c;
        str++;
    }
    return hash;
}

SND_FORCE_INLINE uint32_t snd_hash_fnv1a_lower_internal(const char *str) {
    if (!str)
        return 0;
    uint32_t hash = SND_HASH_SEED;
    while (*str) {
        char c = *str;
        if (c >= 'A' && c <= 'Z') {
            c += ('a' - 'A');
        }
        hash ^= (unsigned char)c;
        hash *= 16777619U;
        str++;
    }
    return hash;
}

SND_FORCE_INLINE uint32_t snd_hash_djb2_wide_lower_internal(const wchar_t *str) {
    if (!str)
        return 0;
    uint32_t hash = SND_HASH_SEED;
    while (*str) {
        wchar_t c = *str;
        if (c >= L'A' && c <= L'Z') {
            c += (L'a' - L'A');
        }
        hash = hash * 33 + (unsigned char)(c & 0xFF);
        str++;
    }
    return hash;
}

SND_FORCE_INLINE uint32_t snd_hash_fnv1a_wide_lower_internal(const wchar_t *str) {
    if (!str)
        return 0;
    uint32_t hash = SND_HASH_SEED;
    while (*str) {
        wchar_t c = *str;
        if (c >= L'A' && c <= L'Z') {
            c += (L'a' - L'A');
        }
        hash ^= (unsigned char)(c & 0xFF);
        hash *= 16777619U;
        str++;
    }
    return hash;
}

uint32_t snd_hash(const char *str) {
#if defined(SND_HASH_FNV1A)
    return snd_hash_fnv1a_internal(str);
#else
    return snd_hash_djb2_internal(str);
#endif
}

uint32_t snd_hash_lower(const char *str) {
#if defined(SND_HASH_FNV1A)
    return snd_hash_fnv1a_lower_internal(str);
#else
    return snd_hash_djb2_lower_internal(str);
#endif
}

uint32_t snd_hash_wide_lower(const wchar_t *str) {
#if defined(SND_HASH_FNV1A)
    return snd_hash_fnv1a_wide_lower_internal(str);
#else
    return snd_hash_djb2_wide_lower_internal(str);
#endif
}
