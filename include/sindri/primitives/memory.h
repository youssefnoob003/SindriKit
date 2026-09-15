#ifndef SND_PRIMITIVES_MEMORY_H
#define SND_PRIMITIVES_MEMORY_H

#include <sindri/common/buffer.h>
#include <sindri/common/macros.h>
#include <sindri/primitives/os_api.h>

SND_BEGIN_EXTERN_C

// Expose globally available, ready-to-use WinAPI capabilities
extern const snd_memory_api_t snd_mem_win;
extern const snd_memory_api_t snd_mem_nt;
extern const snd_memory_api_t snd_mem_sys;

/**
 * @brief Releases a buffer allocated by the Win32 memory backend.
 */
void snd_buffer_free_win(snd_buffer_t *buffer);

/**
 * @brief Releases a buffer allocated by the native NTDLL memory backend.
 */
void snd_buffer_free_nt(snd_buffer_t *buffer);

/**
 * @brief Releases a buffer allocated by the direct-syscall memory backend.
 */
void snd_buffer_free_sys(snd_buffer_t *buffer);

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_MEMORY_H
