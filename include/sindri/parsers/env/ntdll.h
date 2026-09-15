#ifndef SND_PARSERS_ENV_NTDLL_H
#define SND_PARSERS_ENV_NTDLL_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/types.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/status/core.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Internal structure holding NTDLL parser state and base address.
 */
typedef struct {
    PVOID           base;
    snd_pe_parser_t parser;
    BOOL            is_initialized;
} snd_ntdll_entry_t;

/**
 * @brief Sets a custom clean NTDLL image base address for SSN resolution and gadget scanning.
 *
 * Parses the provided in-memory NTDLL image and initializes internal context for clean inspections.
 *
 * @param clean_base Virtual memory address of the loaded/mapped clean NTDLL image.
 *
 * @retval SND_OK On successfully parsing and storing the clean NTDLL context.
 * @retval SND_STATUS_NULL_POINTER If `clean_base` is NULL.
 * @retval Any error returned by `snd_pe_parse`.
 */
snd_status_t snd_ntdll_set_clean(PVOID clean_base);

/**
 * @brief Retrieves the pre-parsed PE parser context for the active executable PEB NTDLL.
 *
 * Lazily locates NTDLL from the process PEB on first call and initializes its parser context.
 *
 * @param out_parser Pointer to receive the pointer to the active PEB NTDLL parser context.
 *
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If `out_parser` is NULL.
 * @retval Any error returned by `snd_peb_get_module_base_hash` or
 * `snd_pe_parse`.
 */
snd_status_t snd_ntdll_get_active_parser(const snd_pe_parser_t **out_parser);

/**
 * @brief Retrieves the active process PEB NTDLL base memory address.
 *
 * @param out_base Pointer to receive the base virtual address of active NTDLL.
 *
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If `out_base` is NULL.
 * @retval Any error returned by `snd_ntdll_get_active_parser`.
 */
snd_status_t snd_ntdll_get_active_base(PVOID *out_base);

/**
 * @brief Resolves an export function address from active PEB NTDLL by DJB2 string hash.
 *
 * Used by runtime wrappers to locate native syscall procedures inside executable virtual memory.
 *
 * @param func_hash The 32-bit DJB2 hash of the exported function name.
 * @param func_addr_out Pointer to receive the resolved procedure address (`FARPROC`).
 *
 * @retval SND_OK On successfully locating the exported procedure address.
 * @retval Any error returned by `snd_ntdll_get_active_parser` or
 * `snd_pe_get_export_address_hash`.
 */
snd_status_t snd_ntdll_get_active_export(DWORD func_hash, FARPROC *func_addr_out);

/**
 * @brief Retrieves the PE parser context for the clean NTDLL target image.
 *
 * Requires `snd_ntdll_set_clean` to have been called beforehand.
 *
 * @param out_parser Pointer to receive the pointer to the clean NTDLL parser context.
 *
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If `out_parser` is NULL.
 * @retval SND_STATUS_NTDLL_CLEAN_NOT_INITIALIZED If clean NTDLL base has not been explicitly configured.
 */
snd_status_t snd_ntdll_get_clean_parser(const snd_pe_parser_t **out_parser);

/**
 * @brief Retrieves the base memory address of the clean NTDLL target image.
 *
 * @param out_base Pointer to receive the clean NTDLL base address.
 *
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If `out_base` is NULL.
 * @retval SND_STATUS_NTDLL_CLEAN_NOT_INITIALIZED If clean NTDLL base has not been explicitly configured.
 */
snd_status_t snd_ntdll_get_clean_base(PVOID *out_base);

/**
 * @brief Resolves an export function address from the clean NTDLL target by DJB2 string hash.
 *
 * Used by system resolvers to inspect unhooked/clean NTDLL images for SSN extraction or code validation.
 *
 * @param func_hash The 32-bit DJB2 hash of the target exported function.
 * @param func_addr_out Pointer to receive the resolved function address (`FARPROC`).
 *
 * @retval SND_OK On successfully resolving the function address from clean NTDLL.
 * @retval Any error returned by `snd_ntdll_get_clean_parser` or
 * `snd_pe_get_export_address_hash`.
 */
snd_status_t snd_ntdll_get_clean_export(DWORD func_hash, FARPROC *func_addr_out);

SND_END_EXTERN_C

#endif // SND_PARSERS_ENV_NTDLL_H
