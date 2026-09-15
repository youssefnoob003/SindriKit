#ifndef SND_PARSERS_COFF_SYMBOLS_H
#define SND_PARSERS_COFF_SYMBOLS_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/coff.h>
#include <sindri/parsers/coff/parser.h>
#include <sindri/status/core.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Maximum symbol length for COFF symbol name extraction
 */
#define SND_COFF_MAX_SYMBOL_LEN 4096

/**
 * @brief Defines the resolved runtime category of a COFF symbol.
 */
typedef enum {
    SND_COFF_SYM_TYPE_LOCAL,
    SND_COFF_SYM_TYPE_BSS,
    SND_COFF_SYM_TYPE_IMPORT,
    SND_COFF_SYM_TYPE_OTHER
} snd_coff_sym_type_t;

/**
 * @brief Contains metadata extracted from an external import symbol.
 *
 * @note The engine extracts these values from specially crafted COFF names
 * like `__imp_DLLNAME$FunctionName`.
 */
SND_SHUFFLE_START
typedef struct {
    char dll_name[128];
    char func_name[128];
    BOOL is_imp;
} snd_coff_import_info_t;
SND_SHUFFLE_END

/**
 * @brief A standardized container representing a fully decoded COFF symbol.
 *
 * @note This structure is populated by `snd_coff_decode_symbol` and safely
 * handles bounded strings to prevent buffer overruns during resolution.
 */
SND_SHUFFLE_START
typedef struct {
    snd_coff_sym_type_t    type;
    snd_coff_import_info_t import;
    SIZE_T                 bss_size;
} snd_coff_decoded_sym_t;
SND_SHUFFLE_END

/**
 * @brief Retrieves a symbol by its zero-based index in the symbol table.
 *
 * Note: Symbols can have auxiliary records which take up index slots.
 * This function returns the raw symbol at the exact index, but it is up to
 * the caller to manage skipping aux records if iterating manually.
 *
 * @param parser The parsed COFF object.
 * @param index The zero-based index.
 * @retval Pointer to the IMAGE_SYMBOL.
 * @retval NULL If the index is invalid or outside the symbol table.
 */
PSND_IMAGE_SYMBOL snd_coff_get_symbol_by_index(const snd_coff_parser_t *parser, DWORD index);

/**
 * @brief Finds a symbol by its exact string name.
 *
 * @param parser The parsed COFF object.
 * @param name The name to search for (e.g. "_MyFunction").
 * @param symbol_out Output pointer to store the located symbol.
 * @param name_len The length of the name to search for.
 * @param index_out Optional output pointer to store the located symbol's index.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If an argument is NULL.
 * @retval SND_STATUS_SYMBOL_ENTRY_MISSING If a symbol record is unavailable.
 * @retval Any error returned by `snd_coff_get_symbol_name`.
 */
snd_status_t snd_coff_find_symbol_by_name(const snd_coff_parser_t *parser, const char *name, SIZE_T name_len,
                                          PSND_IMAGE_SYMBOL *symbol_out, DWORD *index_out);

/**
 * @brief Decodes a symbol into a human-readable format.
 *
 * @param parser The parsed COFF object.
 * @param sym The symbol to decode.
 * @param decoded Output pointer to store the decoded symbol.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If an argument is NULL.
 * @retval SND_STATUS_SYMBOL_NAKED_REJECTED If the symbol format is unsupported.
 * @retval SND_STATUS_SYMBOL_TABLE_OVERFLOW If symbol-table arithmetic overflows.
 * @retval SND_STATUS_SYMBOL_DECODE_OVERFLOW If decoding exceeds the available
 * symbol data.
 * @retval Any error returned by `snd_coff_get_symbol_name`.
 */
snd_status_t snd_coff_decode_symbol(const snd_coff_parser_t *parser, PSND_IMAGE_SYMBOL sym,
                                    snd_coff_decoded_sym_t *decoded);

SND_END_EXTERN_C

#endif // SND_PARSERS_COFF_SYMBOLS_H
