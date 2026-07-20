#ifndef SND_PARSERS_COFF_SYMBOLS_H
#define SND_PARSERS_COFF_SYMBOLS_H

#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/parsers/coff/parser.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

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
 * @return Pointer to the IMAGE_SYMBOL, or NULL if invalid/out of bounds.
 */
PIMAGE_SYMBOL snd_coff_get_symbol_by_index(const snd_coff_parser_t *parser, DWORD index);

/**
 * @brief Finds a symbol by its exact string name.
 *
 * @param parser The parsed COFF object.
 * @param name The name to search for (e.g. "_MyFunction").
 * @param symbol_out Output pointer to store the located symbol.
 * @param index_out Optional output pointer to store the located symbol's index.
 * @return SND_OK on success, SND_STATUS_NOT_FOUND, or other errors.
 */
snd_status_t snd_coff_find_symbol_by_name(const snd_coff_parser_t *parser, const char *name, PIMAGE_SYMBOL *symbol_out,
                                          DWORD *index_out);

/**
 * @brief Decodes a symbol into a human-readable format.
 *
 * @param parser The parsed COFF object.
 * @param sym The symbol to decode.
 * @param decoded Output pointer to store the decoded symbol.
 * @return SND_OK on success, or an error code on failure.
 */
snd_status_t snd_coff_decode_symbol(const snd_coff_parser_t *parser, PIMAGE_SYMBOL sym,
                                    snd_coff_decoded_sym_t *decoded);

/**
 * @brief Writes a jump trampoline to the destination address.
 *
 * @param dest The destination address to write the trampoline to.
 * @param target_addr The target address to jump to.
 */
void snd_coff_write_jmp_trampoline(void *dest, void *target_addr);

SND_END_EXTERN_C

#endif // SND_PARSERS_COFF_SYMBOLS_H
