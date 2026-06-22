#ifndef SND_PRIMITIVES_OS_API_H
#define SND_PRIMITIVES_OS_API_H

#include <sindri/common/helpers.h>
#include <sindri/common/status.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

// Memory Capabilities
typedef snd_status_t(WINAPI *snd_memory_alloc_cb)(LPVOID address, SIZE_T size, DWORD allocation_type, DWORD protect,
                                                  LPVOID *out_address);
typedef snd_status_t(WINAPI *snd_memory_free_cb)(LPVOID address, SIZE_T size, DWORD free_type);
typedef snd_status_t(WINAPI *snd_memory_protect_cb)(LPVOID address, SIZE_T size, DWORD new_protect, DWORD *old_protect);

// Module Capabilities
typedef snd_status_t(WINAPI *snd_module_load_cb)(const char *module_name, HMODULE *out_module);
typedef snd_status_t(WINAPI *snd_module_get_proc_cb)(HMODULE hModule, const char *proc_name, FARPROC *out_proc);
typedef snd_status_t(WINAPI *snd_module_resolver_cb)(const wchar_t *module_name, PVOID *out_base);

// Module Capabilities (Hashes)
typedef snd_status_t(WINAPI *snd_module_load_hash_cb)(DWORD module_hash, HMODULE *out_module);
typedef snd_status_t(WINAPI *snd_module_get_proc_hash_cb)(HMODULE hModule, DWORD proc_hash, FARPROC *out_proc);
typedef snd_status_t(WINAPI *snd_module_resolver_hash_cb)(DWORD module_hash, PVOID *out_base);

/**
 * @brief Local Memory Management API table.
 */
typedef struct {
    snd_memory_alloc_cb   alloc;
    snd_memory_free_cb    free;
    snd_memory_protect_cb protect;
} snd_memory_api_t;

/**
 * @brief Module and Import Resolution API table.
 */
typedef struct {
    // String-based resolution
    snd_module_load_cb     load_library;
    snd_module_get_proc_cb get_proc_address;
    snd_module_resolver_cb get_module_base;

    // Hash-based resolution
    snd_module_load_hash_cb     load_library_hash;
    snd_module_get_proc_hash_cb get_proc_address_hash;
    snd_module_resolver_hash_cb get_module_base_hash;
} snd_module_api_t;

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_OS_API_H
