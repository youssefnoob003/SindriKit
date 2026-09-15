#ifndef SND_PRIMITIVES_PROCESS_H
#define SND_PRIMITIVES_PROCESS_H

#include <sindri/common/macros.h>
#include <sindri/primitives/os_api.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

snd_status_t WINAPI snd_nt_create_process_custom(const wchar_t *image_path, const wchar_t *command_line,
                                                 HANDLE *out_process, HANDLE *out_thread,
                                                 snd_process_create_params_cb create_cb,
                                                 snd_process_free_params_cb   free_cb);

snd_status_t WINAPI snd_sys_create_process_custom(const wchar_t *image_path, const wchar_t *command_line,
                                                  HANDLE *out_process, HANDLE *out_thread,
                                                  snd_process_create_params_cb create_cb,
                                                  snd_process_free_params_cb   free_cb);

// Expose globally available, ready-to-use WinAPI capabilities
extern const snd_process_api_t snd_proc_win;
extern const snd_process_api_t snd_proc_nt;
extern const snd_process_api_t snd_proc_sys;

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_PROCESS_H
