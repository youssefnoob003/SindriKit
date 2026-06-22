#ifndef SND_PARSERS_PE_EXPORTS_H
#define SND_PARSERS_PE_EXPORTS_H

#include "sindri_hashes.h"

#include <sindri/common/helpers.h>
#include <sindri/common/status.h>
#include <sindri/primitives/os_api.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Maximum recursion depth for resolving export forwarders.
 * Prevents stack overflows from malicious or circular forwarder chains.
 */
#define SND_FWD_MAX_DEPTH 4

/**
 * @brief Resolves the memory address of an exported function by name or
 * ordinal.
 *
 * This function parses the Export Address Table (EAT). It supports resolving
 * functions by name, by raw ordinal (using `INTRESOURCE`), and automatically
 * handles NT Export Forwarders (e.g., forwarding a call to another DLL).
 *
 * @param base_address The base address of the mapped PE image.
 * @param size The size of the mapped PE image. Can be
 * `SND_SYS_DLL_SIZE_DEFAULT` to dynamically infer the size from the PE
 * headers.
 * @param func_name The name of the exported function, or a pointer-sized
 * ordinal generated via the `MAKEINTRESOURCE` macro.
 * @param func_addr_out Pointer that receives the resolved memory address on
 * success.
 * @param resolver Callback function used to load external dependencies if the
 * requested export is a forwarder. If NULL, forwarder resolution will fail.
 * @returns SND_OK on success, or an error status.
 */
snd_status_t snd_pe_get_export_address(PVOID base_address, SIZE_T size, const char *func_name, FARPROC *func_addr_out,
                                       snd_module_resolver_cb resolver);

/**
 * @brief Resolves the memory address of an exported function using a DJB2 hash.
 *
 * An API-hashing alternative to `snd_pe_get_export_address` for use in
 * stealthy or position-independent contexts where plain-text strings are
 * avoided.
 *
 * @param base_address The base address of the mapped PE image.
 * @param size The size of the mapped PE image.
 * @param func_hash The custom hash of the exported function name.
 * @param func_addr_out Pointer that receives the resolved memory address on
 * success.
 * @param resolver Callback function used to resolve external dependencies via
 * hash if the requested export is a forwarder. If NULL, forwarders fail.
 * @returns SND_OK on success, or an error status.
 */
snd_status_t snd_pe_get_export_address_by_hash(PVOID base_address, SIZE_T size, DWORD func_hash, FARPROC *func_addr_out,
                                               snd_module_resolver_hash_cb resolver);

SND_END_EXTERN_C

#endif // SND_PARSERS_PE_EXPORTS_H
