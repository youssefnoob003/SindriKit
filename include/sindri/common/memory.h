#ifndef SND_COMMON_MEMORY_H
#define SND_COMMON_MEMORY_H

#include <sindri/common/macros.h>
#include <stddef.h>
#include <stdint.h>

SND_BEGIN_EXTERN_C

/**
 * @def SND_PTR_ADD
 * @brief Safely adds a byte offset to a base pointer.
 */
#define SND_PTR_ADD(base, offset) ((unsigned char *)(base) + (offset))

/**
 * @brief Checks if a given size and offset are within the bounds of a total size.
 * @return 1 if within bounds, 0 otherwise.
 */
SND_FORCE_INLINE int snd_memory_bounds_check(size_t total_size, size_t offset, size_t size) {
    if (offset > total_size)
        return 0;
    if (size > total_size - offset)
        return 0;
    return 1;
}

/**
 * @brief Checks if a pointer and size are within the bounds of a base pointer and total size.
 * @return 1 if within bounds, 0 otherwise.
 */
SND_FORCE_INLINE int snd_memory_ptr_bounds_check(const void *base, size_t total_size, const void *ptr, size_t size) {
    if (base == NULL || ptr == NULL)
        return 0;
    uintptr_t ubase   = (uintptr_t)base;
    uintptr_t utarget = (uintptr_t)ptr;
    if (utarget < ubase)
        return 0;
    size_t offset = (size_t)(utarget - ubase);
    return snd_memory_bounds_check(total_size, offset, size);
}

/**
 * @brief Bounded, CRT-independent memory zeroing routine. Replaces memset.
 */
SND_FORCE_INLINE void snd_memzero(void *dest, size_t size) {
    if (!dest)
        return;
    volatile unsigned char *p = (volatile unsigned char *)dest;
    for (size_t i = 0; i < size; i++) {
        p[i] = 0;
    }
}

/**
 * @brief Raw byte copy routine. Replaces memcpy.
 */
SND_FORCE_INLINE void snd_memcpy(void *dest, const void *src, size_t count) {
    if (!dest || !src)
        return;
    unsigned char       *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
}

SND_END_EXTERN_C

#endif // SND_COMMON_MEMORY_H
