#ifndef SND_PRIMITIVES_THREAD_H
#define SND_PRIMITIVES_THREAD_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/types.h>
#include <sindri/primitives/os_api.h>
#include <sindri/status.h>

/**
 * @brief Portable 1:1 projection of a native thread context.
 */
struct snd_thread_registers {
    ULONG_PTR ip;
    ULONG_PTR sp;
    ULONG_PTR cx;
    ULONG_PTR dx;
    DWORD     rflags;
};

SND_BEGIN_EXTERN_C

// Expose globally available, ready-to-use WinAPI capabilities
extern const snd_thread_api_t snd_thread_win;
extern const snd_thread_api_t snd_thread_nt;
extern const snd_thread_api_t snd_thread_sys;

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_THREAD_H
