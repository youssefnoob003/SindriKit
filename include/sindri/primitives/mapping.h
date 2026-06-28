#ifndef SND_PRIMITIVES_MAPPING_H
#define SND_PRIMITIVES_MAPPING_H

#include <sindri/common/macros.h>
#include <sindri/primitives/os_api.h>

SND_BEGIN_EXTERN_C

// Expose globally available, ready-to-use WinAPI capabilities
extern const snd_mapping_api_t snd_map_win;
extern const snd_mapping_api_t snd_map_nt;
extern const snd_mapping_api_t snd_map_sys;

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_MAPPING_H
