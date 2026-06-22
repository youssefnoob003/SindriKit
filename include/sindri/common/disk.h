#ifndef SND_COMMON_DISK_H
#define SND_COMMON_DISK_H

#include <sindri/common/buffer.h>
#include <sindri/common/helpers.h>
#include <sindri/common/status.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Reads an entire file into a heap-allocated buffer.
 * @param path The null-terminated string of the file path to read.
 * @param out_buf Pointer to a zeroed snd_buffer_t. On success, data and size
 * are populated.
 * @return SND_OK on success, otherwise an I/O related error code.
 * @note The caller is responsible for freeing the buffer using
 * snd_buffer_free(out_buf).
 */
snd_status_t snd_buffer_load_from_disk(const char *path, snd_buffer_t *out_buf);

SND_END_EXTERN_C

#endif // SND_COMMON_DISK_H
