#ifndef SND_PARSERS_PE_EXPORTS_H
#define SND_PARSERS_PE_EXPORTS_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/pe.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/primitives/os_api.h>
#include <sindri/status/core.h>
#include <sindri_hashes.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Maximum recursion depth for resolving export forwarders.
 * Prevents stack overflows and infinite loop execution caused by circular
 * forwarder chains or malicious export tables.
 */
#define SND_FWD_MAX_DEPTH 4

/**
 * @brief Resolves the memory address of an exported function by name or ordinal.
 *
 * Parses the Export Address Table (EAT) of a parsed PE image. Supports standard
 * string lookups, ordinal lookups (when passed via the `SND_IS_INTRESOURCE` /
 * `SND_MAKEINTRESOURCE` macro), and recursive resolution of NT Export Forwarders.
 *
 * @param parser Pointer to the initialized PE parser context.
 * @param func_name Null-terminated function name string, or a 16-bit ordinal
 *                  value formatted with `SND_MAKEINTRESOURCE`.
 * @param func_addr_out Pointer to receive the resolved function address (`FARPROC`).
 * @param resolver Optional callback used to load external modules if the export
 *                 is forwarded. Required if resolving forwarded exports; if NULL,
 *                 forwarder resolution will fail.
 *
 * @retval SND_OK On successfully resolving the function memory address.
 * @retval SND_STATUS_INVALID_PARAMETERS_COMBINATION If `parser`, `func_addr_out`, or `func_name` is NULL.
 * @retval SND_STATUS_EXPORT_DIRECTORY_NULL If the export data directory is missing or unmapped.
 * @retval SND_STATUS_EXPORT_TABLE_EMPTY If the export directory contains zero functions.
 * @retval SND_STATUS_EXPORT_SYMBOL_MISSING If the requested function name or ordinal was not found.
 * @retval SND_STATUS_EXPORT_DIRECTORY_INVALID If table RVAs point to invalid memory outside image bounds.
 * @retval SND_STATUS_EXPORT_DIRECTORY_OVERFLOW If calculating table offsets results in integer overflow.
 * @retval SND_STATUS_EXPORT_ORDINAL_INVALID If ordinal value is 0, out of 16-bit range, or malformed.
 * @retval SND_STATUS_EXPORT_ORDINAL_OUT_OF_RANGE If ordinal is lower than `exp_dir->Base` or exceeds max index.
 * @retval SND_STATUS_EXPORT_ORDINAL_OVERFLOW If ordinal calculations trigger integer arithmetic overflow.
 * @retval SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED If export is forwarded but `resolver` is NULL.
 * @retval SND_STATUS_EXPORT_FORWARDER_EXCEEDED If export forwarder chain depth exceeds `SND_FWD_MAX_DEPTH`.
 * @retval SND_STATUS_EXPORT_FORWARDER_INVALID If forwarder string is malformed, untruncated, or exceeds bounds.
 * @retval Any error returned by `snd_pe_get_directory`, the
 * `snd_module_resolver_cb` callback, or `snd_pe_parse` while resolving a
 * forwarded export.
 */
snd_status_t snd_pe_get_export_address(const snd_pe_parser_t *parser, const char *func_name, FARPROC *func_addr_out,
                                       snd_module_resolver_cb resolver);

/**
 * @brief Resolves the memory address of an exported function using a precomputed hash.
 *
 * Alternative to `snd_pe_get_export_address` for stealth or obfuscated contexts
 * where plain-text function names are omitted. Iterates named exports and computes
 * string hashes to locate the symbol.
 *
 * @param parser Pointer to the initialized PE parser context.
 * @param func_hash The 32-bit hash value of the target function name.
 * @param func_addr_out Pointer to receive the resolved function address (`FARPROC`).
 * @param resolver Optional callback used to load external modules for forwarded exports.
 *
 * @retval SND_OK On successfully resolving the hashed symbol memory address.
 * @retval SND_STATUS_NULL_POINTER If `parser` or `func_addr_out` is NULL.
 * @retval SND_STATUS_INVALID_PARAMETERS_COMBINATION If `func_hash` is 0.
 * @retval SND_STATUS_EXPORT_DIRECTORY_NULL If the export data directory is missing or unmapped.
 * @retval SND_STATUS_EXPORT_TABLE_EMPTY If the export directory contains zero functions.
 * @retval SND_STATUS_EXPORT_SYMBOL_MISSING If no exported symbol matches `func_hash`.
 * @retval SND_STATUS_EXPORT_DIRECTORY_INVALID If table RVAs point to invalid memory outside image bounds.
 * @retval SND_STATUS_EXPORT_DIRECTORY_OVERFLOW If calculating table offsets results in integer overflow.
 * @retval SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED If export is forwarded but `resolver` is NULL.
 * @retval SND_STATUS_EXPORT_FORWARDER_EXCEEDED If export forwarder chain depth exceeds `SND_FWD_MAX_DEPTH`.
 * @retval SND_STATUS_EXPORT_FORWARDER_INVALID If forwarder string is malformed, untruncated, or exceeds bounds.
 * @retval Any error returned by `snd_pe_get_directory`, the
 * `snd_module_resolver_cb` callback, or `snd_pe_parse` while resolving a
 * forwarded export.
 */
snd_status_t snd_pe_get_export_address_hash(const snd_pe_parser_t *parser, DWORD func_hash, FARPROC *func_addr_out,
                                            snd_module_resolver_cb resolver);

/**
 * @brief Callback function type for PE export table enumeration.
 *
 * @param func_name Name of the exported symbol (or NULL if exported purely by ordinal).
 * @param ordinal Absolute ordinal number of the exported function.
 * @param func_addr Resolved virtual memory pointer to the export implementation.
 * @param user_ctx User-provided arbitrary context pointer.
 * @retval TRUE Continue iterating through remaining exports.
 * @retval FALSE Stop enumeration immediately.
 */
typedef BOOL (*snd_pe_export_enum_cb)(const char *func_name, WORD ordinal, PVOID func_addr, PVOID user_ctx);

/**
 * @brief Enumerates all named exported symbols from a parsed PE image.
 *
 * Traverses the export directory, mapping each named export back to its ordinal
 * and target RVA, then invokes the provided callback function for each entry.
 *
 * @param parser Pointer to the initialized PE parser context.
 * @param callback Callback function invoked for every valid export entry.
 * @param user_ctx Arbitrary pointer passed directly through to the callback.
 *
 * @retval SND_OK On successful enumeration completion or if export count is 0.
 * @retval SND_STATUS_NULL_POINTER If `parser` or `callback` is NULL.
 * @retval SND_STATUS_EXPORT_DIRECTORY_NULL If the export directory directory/pointer is invalid.
 * @retval SND_STATUS_EXPORT_DIRECTORY_INVALID If `AddressOfFunctions` pointer fails RVA conversion.
 * @retval Any error returned by `snd_pe_get_directory`.
 */
snd_status_t snd_pe_enumerate_exports(const snd_pe_parser_t *parser, snd_pe_export_enum_cb callback, PVOID user_ctx);

SND_END_EXTERN_C

#endif // SND_PARSERS_PE_EXPORTS_H
