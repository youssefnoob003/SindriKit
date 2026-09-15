#ifndef SND_COMMON_MEMORY_H
#define SND_COMMON_MEMORY_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/types.h>
#include <stddef.h>
#include <stdint.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Safely adds a byte offset to a base pointer.
 */
#define SND_PTR_ADD(base, offset) ((unsigned char *)(base) + (offset))

/**
 * @brief Safely subtracts a byte offset from a base pointer.
 */
#define SND_PTR_SUB(base, offset) ((unsigned char *)(base) - (offset))

/**
 * @brief Calculates the signed byte delta between an actual base address and a preferred base address.
 */
#define SND_PTR_DELTA(target_base, preferred_base) ((LONG_PTR)((ULONG_PTR)(target_base) - (ULONG_PTR)(preferred_base)))

/**
 * @brief Calculates the byte distance between two pointers.
 */
#define SND_PTR_DIFF(ptr_high, ptr_low) ((size_t)((unsigned char *)(ptr_high) - (unsigned char *)(ptr_low)))

#define SND_LOW_WORD(value)  ((WORD)((ULONG_PTR)(value) & 0xffffu))
#define SND_HIGH_WORD(value) ((WORD)(((ULONG_PTR)(value) >> 16) & 0xffffu))

#define SND_MAX_SIZE_T ((SIZE_T) - 1)
#define SND_PAGE_SIZE  ((SIZE_T)0x1000)

/**
 * @brief Checks if a pointer resides strictly before a boundary address.
 */
#define SND_PTR_BEFORE(ptr, boundary) ((ULONG_PTR)(ptr) < (ULONG_PTR)(boundary))

/**
 * @brief Checks if a pointer resides strictly after a boundary address.
 */
#define SND_PTR_AFTER(ptr, boundary) ((ULONG_PTR)(ptr) > (ULONG_PTR)(boundary))

/**
 * @brief Rounds a value down to the nearest multiple of alignment.
 */
#define SND_ALIGN_DOWN(value, alignment) ((alignment) == 0 ? (value) : (((value) / (alignment)) * (alignment)))

/**
 * @def SND_ALIGN_UP
 * @brief Rounds a value up to the nearest multiple of alignment.
 * @note Ensure (value + alignment - 1) does not overflow the underlying integer type before calling.
 */
#define SND_ALIGN_UP(value, alignment)                                                                                 \
    ((alignment) == 0 ? (value) : ((((value) + (alignment) - 1) / (alignment)) * (alignment)))

/**
 * @brief Evaluates to TRUE if the region [offset, offset + length) falls outside [0, max_size).
 * @note Guarded against integer overflow. Do not pass arguments with side effects.
 */
#define SND_RANGE_EXCEEDS(offset, length, max_size)                                                                    \
    ((size_t)(length) > (size_t)(max_size) || (size_t)(offset) > (size_t)(max_size) - (size_t)(length))

/**
 * @brief Evaluates to TRUE if coordinate val resides inside [start, start + size).
 * @note Guarded against integer overflow. Do not pass arguments with side effects.
 */
#define SND_IN_BOUNDS(val, start, size)                                                                                \
    ((size_t)(val) >= (size_t)(start) && (size_t)(val) - (size_t)(start) < (size_t)(size))

/**
 * @brief Checks if adding two positive values will overflow target bounds.
 */
#define SND_ADD_OVERFLOWS_DWORD(a, b) (SND_MAX_DWORD - (DWORD)(a) < (DWORD)(b))
#define SND_ADD_OVERFLOWS_SIZET(a, b) (SND_MAX_SIZE_T - (SIZE_T)(a) < (SIZE_T)(b))

/**
 * @brief Checks if multiplying two positive values will overflow target bounds.
 */
#define SND_MUL_OVERFLOWS_DWORD(a, b) ((b) != 0 && (DWORD)(a) > SND_MAX_DWORD / (DWORD)(b))
#define SND_MUL_OVERFLOWS_SIZET(a, b) ((b) != 0 && (SIZE_T)(a) > SND_MAX_SIZE_T / (SIZE_T)(b))

/**
 * @brief Checks if a given size and offset are within the bounds of a total size.
 * @retval 1 If the range is within bounds.
 * @retval 0 Otherwise.
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
 * @retval 1 If the range is within bounds.
 * @retval 0 Otherwise.
 */
SND_FORCE_INLINE int snd_memory_ptr_bounds_check(const void *base, size_t total_size, const void *ptr, size_t size) {
    if (base == NULL || ptr == NULL)
        return 0;
    ULONG_PTR ubase   = (ULONG_PTR)base;
    ULONG_PTR utarget = (ULONG_PTR)ptr;
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
