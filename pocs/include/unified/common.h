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
    int            cache;
} syscall_style_t;

#if defined(SND_CRTLESS)
#define SND_POC_DEFAULT_BACKEND      API_NT
#define SND_POC_DEFAULT_BACKEND_NAME "--nt"
#else
#define SND_POC_DEFAULT_BACKEND      API_WIN
#define SND_POC_DEFAULT_BACKEND_NAME "--win"
#endif

#endif
