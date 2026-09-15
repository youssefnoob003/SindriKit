#ifndef SND_COMMON_BUFFER_H
#define SND_COMMON_BUFFER_H

#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <stddef.h>

SND_BEGIN_EXTERN_C

// Forward declaration of the buffer structure
typedef struct snd_buffer_s snd_buffer_t;

/**
 * @brief Function signature for a buffer-owned data release callback.
 *
 * The producer that allocates the data supplies this callback. The generic
 * buffer type owns invoking it, but does not assume which allocator created
 * the data.
 */
typedef void (*snd_free_cb)(snd_buffer_t *buf);

/**
 * @brief Represents a tracked, bounds-checked memory buffer.
 */
struct snd_buffer_s {
    void       *data;
    size_t      size;
    snd_free_cb free_routine;
};

/**
 * @brief Initializes a buffer structure with memory tracking context.
 * @param buf The buffer structure to initialize.
 * @param data Pointer to the memory block.
 * @param size Size of the memory block.
 * @param free_routine Optional callback used to release this memory with the
 * allocator that created it.
 */
void snd_buffer_init(snd_buffer_t *buf, void *data, size_t size, snd_free_cb free_routine);

/**
 * @brief Releases buffer data using its assigned callback and zeroes the
 * descriptor.
 * @param buf Buffer descriptor to release.
 */
void snd_buffer_free(snd_buffer_t *buf);

/**
 * @brief Checks if a given offset and size are within the bounds of a buffer.
 * @param buf Buffer to inspect.
 * @param offset Starting byte offset.
 * @param size Number of bytes to validate.
 * @retval 1 If the range is within bounds.
 * @retval 0 Otherwise.
 */
SND_FORCE_INLINE int snd_buffer_bounds_check(const snd_buffer_t *buf, size_t offset, size_t size) {
    if (buf == NULL || buf->data == NULL || buf->size == 0)
        return 0;
    return snd_memory_bounds_check(buf->size, offset, size);
}

SND_END_EXTERN_C

#endif // SND_COMMON_BUFFER_H
