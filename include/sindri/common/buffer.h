#ifndef SND_COMMON_BUFFER_H
#define SND_COMMON_BUFFER_H

#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <stddef.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

// Forward declaration of the buffer structure
typedef struct snd_buffer_s snd_buffer_t;

/**
 * @brief Function signature for the custom buffer deallocator callback.
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
 * @param free_routine Optional callback used to safely deallocate this memory.
 */
void snd_buffer_init(snd_buffer_t *buf, void *data, size_t size, snd_free_cb free_routine);

/**
 * @brief Frees a tracked buffer using its assigned cleanup routine and zeroes the structure.
 */
void snd_buffer_free(snd_buffer_t *buf);

/**
 * @brief Checks if a given offset and size are within the bounds of a buffer.
 * @return 1 if within bounds, 0 otherwise.
 */
SND_FORCE_INLINE int snd_buffer_bounds_check(const snd_buffer_t *buf, size_t offset, size_t size) {
    if (buf == NULL || buf->data == NULL || buf->size == 0)
        return 0;
    return snd_memory_bounds_check(buf->size, offset, size);
}

/**
 * @brief Default routine for buffers allocated via HeapAlloc on the process heap.
 */
SND_FORCE_INLINE void snd_buffer_free_heap(snd_buffer_t *buf) {
    if (buf && buf->data) {
        HeapFree(GetProcessHeap(), 0, buf->data);
    }
}

/**
 * @brief Default routine for buffers allocated via VirtualAlloc.
 */
SND_FORCE_INLINE void snd_buffer_free_virtual(snd_buffer_t *buf) {
    if (buf && buf->data) {
        VirtualFree(buf->data, 0, MEM_RELEASE);
    }
}

/**
 * @brief Default routine for buffers allocated via MapViewOfFile.
 */
SND_FORCE_INLINE void snd_buffer_free_mapped(snd_buffer_t *buf) {
    if (buf && buf->data) {
        UnmapViewOfFile(buf->data);
    }
}

SND_END_EXTERN_C

#endif // SND_COMMON_BUFFER_H
