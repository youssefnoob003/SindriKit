#include <sindri/common/debug.h>
#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <sindri/common/string.h>
#include <sindri/internal/windows/coff.h>
#include <sindri/parsers/coff/parser.h>
#include <sindri/parsers/coff/status.h>
#include <sindri/parsers/coff/symbols.h>
#include <sindri/parsers/coff/utils.h>

PSND_IMAGE_SYMBOL snd_coff_get_symbol_by_index(const snd_coff_parser_t *parser, DWORD index) {
    if (!parser || parser->symbol_table == NULL || index >= parser->symbol_count) {
        return NULL;
    }
    return &parser->symbol_table[index];
}

snd_status_t snd_coff_find_symbol_by_name(const snd_coff_parser_t *parser, const char *name, SIZE_T name_len,
                                          PSND_IMAGE_SYMBOL *symbol_out, DWORD *index_out) {
    SND_CHECK_NULL(parser, name, name_len, symbol_out);

    if (parser->symbol_count == 0 || parser->symbol_table == NULL) {
        return SND_ERR(SND_STATUS_SYMBOL_ENTRY_MISSING);
    }

    char current_name[SND_COFF_MAX_SYMBOL_LEN];

    for (DWORD i = 0; i < parser->symbol_count; i++) {
        PSND_IMAGE_SYMBOL sym = &parser->symbol_table[i];

        snd_status_t status = snd_coff_get_symbol_name(parser, sym, current_name, SND_COFF_MAX_SYMBOL_LEN);
        if (SND_FAILED(status)) {
            return status;
        }

        const char *cmp_name = current_name;
        size_t      cmp_max  = SND_COFF_MAX_SYMBOL_LEN;

        if (cmp_name[0] == '_' && name[0] != '_') {
            cmp_name++;
            cmp_max--;
        }

        size_t cmp_len = snd_strnlen(cmp_name, cmp_max);

        if (cmp_len == name_len && snd_strncmp(name, cmp_name, name_len) == 0) {
            *symbol_out = sym;
            if (index_out) {
                *index_out = i;
            }
            return SND_OK;
        }

        if (sym->NumberOfAuxSymbols > 0) {
            if (SND_ADD_OVERFLOWS_DWORD(i, sym->NumberOfAuxSymbols)) {
                break;
            }
            i += sym->NumberOfAuxSymbols;
        }
    }

    return SND_ERR(SND_STATUS_SYMBOL_ENTRY_MISSING);
}

snd_status_t snd_coff_decode_symbol(const snd_coff_parser_t *parser, PSND_IMAGE_SYMBOL sym,
                                    snd_coff_decoded_sym_t *decoded) {
    SND_CHECK_NULL(parser, sym, decoded);

    snd_memzero(decoded, sizeof(snd_coff_decoded_sym_t));

    if (sym->SectionNumber == SND_IMAGE_SYM_UNDEFINED) {
        if (sym->Value == 0) {
            char         sym_name[SND_COFF_MAX_SYMBOL_LEN];
            snd_status_t status = snd_coff_get_symbol_name(parser, sym, sym_name, SND_COFF_MAX_SYMBOL_LEN);
            if (SND_FAILED(status)) {
                return status;
            }

            BOOL        is_imp      = FALSE;
            const char *target_name = sym_name;

            if (snd_strncmp(target_name, "__imp_", 6) == 0) {
                target_name += 6;
                is_imp = TRUE;
            }

            if (target_name[0] == '_') {
                target_name++;
            }

            size_t      target_len = snd_strnlen(target_name, SND_COFF_MAX_SYMBOL_LEN - (target_name - sym_name));
            const char *dollar     = snd_strnchr(target_name, '$', target_len);

            if (!dollar) {
                SND_DEBUG_PRINT("[coff] naked symbol rejected: '%s'\n", target_name);
                return SND_ERR(SND_STATUS_SYMBOL_NAKED_REJECTED);
            }

            SIZE_T mod_len = dollar - target_name;
            if (mod_len == 0 || mod_len >= sizeof(decoded->import.dll_name)) {
                return SND_ERR(SND_STATUS_SYMBOL_TABLE_OVERFLOW);
            }

            const char *func_start = dollar + 1;
            size_t      func_len   = target_len - mod_len - 1;
            const char *at_sign    = snd_strnchr(func_start, '@', func_len);

            if (at_sign) {
                func_len = at_sign - func_start;
            }

            if (func_len == 0 || func_len >= sizeof(decoded->import.func_name)) {
                return SND_ERR(SND_STATUS_SYMBOL_TABLE_OVERFLOW);
            }

            decoded->type          = SND_COFF_SYM_TYPE_IMPORT;
            decoded->import.is_imp = is_imp;

            snd_strncpy(decoded->import.dll_name, sizeof(decoded->import.dll_name), target_name, mod_len);
            snd_strncpy(decoded->import.func_name, sizeof(decoded->import.func_name), func_start, func_len);

        } else {
            if (sym->Value > (SND_MAX_DWORD - 15)) {
                return SND_ERR(SND_STATUS_SYMBOL_DECODE_OVERFLOW);
            }
            decoded->type     = SND_COFF_SYM_TYPE_BSS;
            decoded->bss_size = (SIZE_T)((sym->Value + 15) & ~((SIZE_T)15));
        }
    } else if (sym->SectionNumber > 0 && (DWORD)sym->SectionNumber <= parser->sections_count) {
        decoded->type = SND_COFF_SYM_TYPE_LOCAL;
    } else {
        decoded->type = SND_COFF_SYM_TYPE_OTHER;
    }

    return SND_OK;
}
