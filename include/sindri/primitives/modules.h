#ifndef SND_PRIMITIVES_MODULES_H
#define SND_PRIMITIVES_MODULES_H

#include <sindri/common/macros.h>
#include <sindri/primitives/os_api.h>

SND_BEGIN_EXTERN_C

// Expose globally available, ready-to-use WinAPI capabilities
extern const snd_module_api_t snd_mod_win;
extern const snd_module_api_t snd_mod_nt;

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_MODULES_H
