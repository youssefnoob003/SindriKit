#ifndef SND_PRIMITIVES_THREAD_H
#define SND_PRIMITIVES_THREAD_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/types.h>
#include <sindri/primitives/os_api.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

// Expose globally available, ready-to-use WinAPI capabilities
extern const snd_thread_api_t snd_thread_win;
extern const snd_thread_api_t snd_thread_nt;
extern const snd_thread_api_t snd_thread_sys;

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_THREAD_H
