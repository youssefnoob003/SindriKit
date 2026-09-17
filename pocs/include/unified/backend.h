#ifndef UNIFIED_BACKEND_H
#define UNIFIED_BACKEND_H

#include <unified/common.h>

typedef struct {
    api_backend_t            backend;
    const snd_file_api_t    *file_api;
    const snd_memory_api_t  *mem_api;
    const snd_module_api_t  *mod_api;
    const snd_process_api_t *proc_api;
    const snd_thread_api_t  *thread_api;
} unified_backend_t;

/**
 * @brief Applies the CLI syscall style to the global pipeline (no-op unless --sys).
 */
void apply_syscall_style(const syscall_style_t *cfg);

/**
 * @brief Binds the selected backend to its primitive tables, bootstrapping the
 * syscall pipeline for API_SYS.
 */
snd_status_t unified_backend_init(api_backend_t backend, const syscall_style_t *scfg, unified_backend_t *out);

/**
 * @brief Loads a file through the bound backend's file API.
 */
snd_status_t unified_file_load(const unified_backend_t *backend, const char *path, snd_buffer_t *out_buffer);

#endif
