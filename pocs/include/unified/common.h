#ifndef UNIFIED_COMMON_H
#define UNIFIED_COMMON_H

#include <sindri.h>
#include <unified/runtime.h>

#if !defined(SND_CRTLESS)
#include <windows.h>
#endif

#define SND_MAX_CALL_ARGS 32

typedef enum {
    API_WIN,
    API_NT,
    API_SYS,
} api_backend_t;

typedef enum {
    INVOKE_DIRECT,
    INVOKE_INDIRECT,
    INVOKE_SPOOFED,
} invoke_style_t;

typedef struct {
    invoke_style_t invoke;
    int            resolve_scan;
    int            resolve_sort;
} syscall_style_t;

static inline snd_status_t unified_file_load(api_backend_t backend, const char *path, snd_buffer_t *out_buffer) {
    if (backend == API_SYS)
        return snd_file_sys.load(path, out_buffer);
    if (backend == API_NT)
        return snd_file_nt.load(path, out_buffer);
#if !defined(SND_CRTLESS)
    return snd_file_win.load(path, out_buffer);
#else
    return SND_ERR_CTX(SND_STATUS_INVALID_COMMAND_LINE_ARG, "Win32 backend is unavailable in CRT-less mode.");
#endif
}

#if defined(SND_CRTLESS)
#define unified_mem_win    snd_mem_nt
#define unified_mod_win    snd_mod_nt
#define unified_proc_win   snd_proc_nt
#define unified_thread_win snd_thread_nt
#else
#define unified_mem_win    snd_mem_win
#define unified_mod_win    snd_mod_win
#define unified_proc_win   snd_proc_win
#define unified_thread_win snd_thread_win
#endif

#endif
