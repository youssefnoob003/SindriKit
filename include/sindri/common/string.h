#ifndef SND_COMMON_STRING_H
#define SND_COMMON_STRING_H

#include <sindri/common/macros.h>
#include <stddef.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Bounded, CRT-independent string length evaluation. Replaces strnlen.
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
 * @brief Bounded, truncation-safe string copying. Replaces strncpy.
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
 * @brief Bounded, CRT-independent string concatenation. Replaces strncat.
 */
SND_FORCE_INLINE void snd_strncat(char *dest, size_t dest_size, const char *src, size_t max_src_len) {
    if (!dest || dest_size == 0 || !src)
        return;
    size_t dest_len = 0;
    while (dest_len < dest_size && dest[dest_len] != '\0') {
        dest_len++;
    }
    if (dest_len >= dest_size - 1) {
        dest[dest_size - 1] = '\0';
        return;
    }
    size_t i = 0;
    while (i < max_src_len && src[i] != '\0' && (dest_len + i) < (dest_size - 1)) {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
}

/**
 * @brief Bounded, CRT-independent character search. Replaces strchr.
 */
SND_FORCE_INLINE const char *snd_strnchr(const char *str, char c, size_t max_len) {
    if (!str)
        return NULL;
    for (size_t i = 0; i < max_len; i++) {
        if (str[i] == c)
            return &str[i];
        if (str[i] == '\0')
            break;
    }
    return NULL;
}

/**
 * @brief Bounded, CRT-independent string comparison. Replaces strncmp.
 */
SND_FORCE_INLINE int snd_strncmp(const char *s1, const char *s2, size_t max_len) {
    if (!s1 || !s2)
        return (s1 == s2) ? 0 : ((s1 < s2) ? -1 : 1);
    for (size_t i = 0; i < max_len; i++) {
        if (s1[i] != s2[i])
            return (int)((unsigned char)s1[i] - (unsigned char)s2[i]);
        if (s1[i] == '\0')
            return 0;
    }
    return 0;
}

/**
 * @brief Bounded, CRT-independent wide string length evaluation. Replaces wcsnlen.
 */
SND_FORCE_INLINE size_t snd_wcsnlen(const wchar_t *str, size_t max_len) {
    if (!str)
        return 0;
    size_t len = 0;
    while (len < max_len && str[len] != L'\0') {
        len++;
    }
    return len;
}

/**
 * @brief Bounded, case-insensitive wide string comparison. Replaces wcsnicmp.
 */
SND_FORCE_INLINE int snd_wcsnicmp(const wchar_t *s1, const wchar_t *s2, size_t n) {
    if (n == 0)
        return 0;
    do {
        wchar_t c1 = *s1++;
        wchar_t c2 = *s2++;
        if (c1 >= L'A' && c1 <= L'Z')
            c1 += (L'a' - L'A');
        if (c2 >= L'A' && c2 <= L'Z')
            c2 += (L'a' - L'A');
        if (c1 != c2)
            return (int)(c1 - c2);
        if (c1 == L'\0')
            break;
    } while (--n);
    return 0;
}

/**
 * @brief Bounded, truncation-safe wide string copying. Replaces wcsncpy.
 */
SND_FORCE_INLINE void snd_wcsncpy(wchar_t *dest, size_t dest_size, const wchar_t *src, size_t max_src_len) {
    if (!dest || dest_size == 0 || !src)
        return;
    size_t i = 0;
    while (i < (dest_size - 1) && i < max_src_len && src[i] != L'\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = L'\0';
}

/**
 * @brief Bounded, CRT-independent wide string concatenation. Replaces wcsncat.
 */
SND_FORCE_INLINE void snd_wcsncat(wchar_t *dest, size_t dest_size, const wchar_t *src, size_t max_src_len) {
    if (!dest || dest_size == 0 || !src)
        return;
    size_t dest_len = 0;
    while (dest_len < dest_size && dest[dest_len] != L'\0') {
        dest_len++;
    }
    if (dest_len >= dest_size - 1) {
        dest[dest_size - 1] = L'\0';
        return;
    }
    size_t i = 0;
    while (i < max_src_len && src[i] != L'\0' && (dest_len + i) < (dest_size - 1)) {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = L'\0';
}

/**
 * @brief Bounded ASCII to Wide string conversion. Replaces mbstowcs.
 */
SND_FORCE_INLINE void snd_ascii_to_wide(wchar_t *dest, size_t dest_size, const char *src, size_t max_src_len) {
    if (!dest || dest_size == 0 || !src)
        return;
    size_t i = 0;
    while (i < (dest_size - 1) && i < max_src_len && src[i] != '\0') {
        dest[i] = (wchar_t)(unsigned char)src[i];
        i++;
    }
    dest[i] = L'\0';
}

SND_END_EXTERN_C

#endif // SND_COMMON_STRING_H
