#ifndef SND_PRIMITIVES_MEMORY_H
#define SND_PRIMITIVES_MEMORY_H

#include <sindri/common/helpers.h>
#include <sindri/primitives/os_api.h>

SND_BEGIN_EXTERN_C

// Expose globally available, ready-to-use WinAPI capabilities
extern const snd_memory_api_t snd_mem_win;
extern const snd_memory_api_t snd_mem_native;

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_MEMORY_H
