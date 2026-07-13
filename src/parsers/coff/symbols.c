#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <sindri/common/status.h>
#include <sindri/common/string.h>
#include <sindri/parsers/coff/parser.h>
#include <sindri/parsers/coff/symbols.h>
#include <sindri/parsers/coff/utils.h>

PIMAGE_SYMBOL snd_coff_get_symbol_by_index(const snd_coff_parser_t *parser, DWORD index) {
    if (!parser || !parser->symbol_table || index >= parser->symbol_count) {
        return NULL;
    }
    return &parser->symbol_table[index];
}

snd_status_t snd_coff_find_symbol_by_name(const snd_coff_parser_t *parser, const char *name, PIMAGE_SYMBOL *symbol_out,
                                          DWORD *index_out) {
    if (!parser || !name || !symbol_out) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    if (parser->symbol_count == 0 || parser->symbol_table == NULL) {
        return SND_ERR(SND_STATUS_COFF_SYMBOL_NOT_FOUND);
    }

    char current_name[256];

    for (DWORD i = 0; i < parser->symbol_count; i++) {
        PIMAGE_SYMBOL sym = &parser->symbol_table[i];

        if (SND_SUCCEEDED(snd_coff_get_symbol_name(parser, sym, current_name, sizeof(current_name)))) {
            const char *cmp_name = current_name;
            if (cmp_name[0] == '_' && name[0] != '_') {
                cmp_name++;
            }

            int match = 1;
            int j     = 0;
            while (name[j] != '\0' && cmp_name[j] != '\0') {
                if (name[j] != cmp_name[j]) {
                    match = 0;
                    break;
                }
                j++;
            }
            if (match && name[j] == '\0' && cmp_name[j] == '\0') {
                *symbol_out = sym;
                if (index_out) {
                    *index_out = i;
                }
                return SND_OK;
            }
        }

        i += sym->NumberOfAuxSymbols;
    }

    return SND_ERR(SND_STATUS_COFF_SYMBOL_NOT_FOUND);
}

snd_status_t snd_coff_decode_symbol(const snd_coff_parser_t *parser, PIMAGE_SYMBOL sym,
                                    snd_coff_decoded_sym_t *decoded) {
    if (!parser || !sym || !decoded) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }
    snd_memzero(decoded, sizeof(snd_coff_decoded_sym_t));

    if (sym->SectionNumber == IMAGE_SYM_UNDEFINED) {
        if (sym->Value == 0) {
            char         sym_name[256];
            snd_status_t status = snd_coff_get_symbol_name(parser, sym, sym_name, sizeof(sym_name));
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

            size_t      target_len = snd_strnlen(target_name, sizeof(sym_name) - (target_name - sym_name));
            const char *dollar     = snd_strnchr(target_name, '$', target_len);

            if (!dollar) {
                return SND_ERR(SND_STATUS_COFF_NAKED_SYMB_REJECTED);
            }

            decoded->type          = SND_COFF_SYM_TYPE_IMPORT;
            decoded->import.is_imp = is_imp;

            SIZE_T mod_len = dollar - target_name;

            if (mod_len >= sizeof(decoded->import.dll_name)) {
                return SND_ERR(SND_STATUS_BUFFER_TOO_SMALL);
            }

            snd_strncpy(decoded->import.dll_name, sizeof(decoded->import.dll_name), target_name, mod_len);
            decoded->import.dll_name[mod_len] = '\0';

            const char *func_start = dollar + 1;
            size_t      func_len   = target_len - mod_len - 1;
            const char *at_sign    = snd_strnchr(func_start, '@', func_len);

            if (at_sign) {
                func_len = at_sign - func_start;
            }

            if (func_len >= sizeof(decoded->import.func_name)) {
                return SND_ERR(SND_STATUS_BUFFER_TOO_SMALL);
            }

            snd_strncpy(decoded->import.func_name, sizeof(decoded->import.func_name), func_start, func_len);
            decoded->import.func_name[func_len] = '\0';

        } else {
            if (sym->Value > (SIZE_T)-1 - 15) {
                return SND_ERR(SND_STATUS_INTEGER_OVERFLOW);
            }
            decoded->type     = SND_COFF_SYM_TYPE_BSS;
            decoded->bss_size = (sym->Value + 15) & ~15;
        }
    } else if (sym->SectionNumber > 0 && (DWORD)sym->SectionNumber <= parser->sections_count) {
        decoded->type = SND_COFF_SYM_TYPE_LOCAL;
    } else {
        decoded->type = SND_COFF_SYM_TYPE_OTHER;
    }

    return SND_OK;
}

void snd_coff_write_jmp_trampoline(void *dest, void *target_addr) {
    unsigned char *tramp = (unsigned char *)dest;
    tramp[0]             = 0xFF;
    tramp[1]             = 0x25;
    tramp[2]             = 0x00;
    tramp[3]             = 0x00;
    tramp[4]             = 0x00;
    tramp[5]             = 0x00;

    snd_memcpy(&tramp[6], &target_addr, sizeof(void *));
}
