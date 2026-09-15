#include <sindri/common/hash.h>
#include <sindri/common/memory.h>
#include <sindri/common/string.h>
#include <sindri/internal/windows/pe.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/parsers/pe/status.h>
#include <sindri/parsers/pe/utils.h>
#include <sindri/primitives/os_api.h>

static snd_status_t pe_get_export_address_impl(const snd_pe_parser_t *parser, const char *func_name, DWORD func_hash,
                                               FARPROC *func_addr_out, snd_module_resolver_cb resolver,
                                               BOOL is_hash_mode, int depth) {
    SND_CHECK_NULL(parser, func_addr_out);
    if (!is_hash_mode && func_name == NULL) {
        return SND_ERR_CTX(SND_STATUS_INVALID_PARAMETERS_COMBINATION,
                           "func_name parameter is NULL in string lookup mode");
    }
    if (is_hash_mode && func_hash == 0) {
        return SND_ERR_CTX(SND_STATUS_INVALID_PARAMETERS_COMBINATION, "func_hash parameter is 0 in hash lookup mode");
    }

    *func_addr_out = NULL;

    SND_IMAGE_DATA_DIRECTORY export_dir = {0};
    SND_TRY(snd_pe_get_directory(parser, SND_IMAGE_DIRECTORY_ENTRY_EXPORT, &export_dir));

    PSND_IMAGE_EXPORT_DIRECTORY exp_dir = (PSND_IMAGE_EXPORT_DIRECTORY)snd_pe_rva_to_ptr(
        parser, export_dir.VirtualAddress, sizeof(SND_IMAGE_EXPORT_DIRECTORY));
    if (!exp_dir) {
        return SND_ERR(SND_STATUS_EXPORT_DIRECTORY_NULL);
    }
    if (exp_dir->NumberOfFunctions == 0) {
        return SND_ERR(SND_STATUS_EXPORT_TABLE_EMPTY);
    }

    DWORD func_rva          = 0;
    BOOL  is_ordinal_lookup = (!is_hash_mode && func_name != NULL && SND_IS_INTRESOURCE(func_name));

    if (is_ordinal_lookup) {
        ULONG_PTR requested_ordinal = (ULONG_PTR)func_name;

        if (requested_ordinal == 0 || requested_ordinal > SND_MAX_WORD) {
            return SND_ERR_CTX(SND_STATUS_EXPORT_ORDINAL_INVALID, "Ordinal value out of 16-bit range");
        }

        ULONG_PTR base_ord = exp_dir->Base;
        if (!SND_IN_BOUNDS(requested_ordinal, base_ord, exp_dir->NumberOfFunctions)) {
            return SND_ERR(SND_STATUS_EXPORT_ORDINAL_OUT_OF_RANGE);
        }

        DWORD func_idx = (DWORD)(requested_ordinal - base_ord);
        if (SND_MUL_OVERFLOWS_DWORD(func_idx, sizeof(DWORD))) {
            return SND_ERR(SND_STATUS_EXPORT_ORDINAL_OVERFLOW);
        }

        DWORD func_offset = func_idx * (DWORD)sizeof(DWORD);
        if (SND_ADD_OVERFLOWS_DWORD(exp_dir->AddressOfFunctions, func_offset)) {
            return SND_ERR(SND_STATUS_EXPORT_ORDINAL_OVERFLOW);
        }

        DWORD *p_func_rva =
            (DWORD *)snd_pe_rva_to_ptr(parser, exp_dir->AddressOfFunctions + func_offset, sizeof(DWORD));
        if (!p_func_rva) {
            return SND_ERR(SND_STATUS_EXPORT_DIRECTORY_INVALID);
        }
        func_rva = *p_func_rva;
    } else {
        for (DWORD i = 0; i < exp_dir->NumberOfNames; i++) {
            if (SND_MUL_OVERFLOWS_DWORD(i, sizeof(DWORD))) {
                return SND_ERR(SND_STATUS_EXPORT_DIRECTORY_OVERFLOW);
            }
            DWORD name_offset = i * (DWORD)sizeof(DWORD);
            if (SND_ADD_OVERFLOWS_DWORD(exp_dir->AddressOfNames, name_offset)) {
                return SND_ERR(SND_STATUS_EXPORT_DIRECTORY_OVERFLOW);
            }

            DWORD *p_name_rva =
                (DWORD *)snd_pe_rva_to_ptr(parser, exp_dir->AddressOfNames + name_offset, sizeof(DWORD));
            if (!p_name_rva) {
                return SND_ERR(SND_STATUS_EXPORT_DIRECTORY_INVALID);
            }
            DWORD name_rva = *p_name_rva;

            const char *current_name = (char *)snd_pe_rva_to_ptr(parser, name_rva, 1);
            if (!current_name) {
                continue;
            }

            SIZE_T name_offset_bytes = SND_PTR_DIFF(current_name, parser->source.data);
            SIZE_T max_safe_length   = parser->source.size - name_offset_bytes;

            if (snd_strnlen(current_name, max_safe_length) == max_safe_length) {
                continue;
            }

            if (!is_hash_mode) {
                if (snd_strncmp(current_name, func_name, max_safe_length) != 0) {
                    continue;
                }
            } else {
                if (snd_hash(current_name) != func_hash) {
                    continue;
                }
            }

            if (SND_MUL_OVERFLOWS_DWORD(i, sizeof(WORD))) {
                return SND_ERR(SND_STATUS_EXPORT_DIRECTORY_OVERFLOW);
            }
            DWORD ord_offset = i * (DWORD)sizeof(WORD);
            if (SND_ADD_OVERFLOWS_DWORD(exp_dir->AddressOfNameOrdinals, ord_offset)) {
                return SND_ERR(SND_STATUS_EXPORT_DIRECTORY_OVERFLOW);
            }

            WORD *p_ordinal =
                (WORD *)snd_pe_rva_to_ptr(parser, exp_dir->AddressOfNameOrdinals + ord_offset, sizeof(WORD));
            if (!p_ordinal) {
                return SND_ERR(SND_STATUS_EXPORT_DIRECTORY_INVALID);
            }
            WORD ordinal = *p_ordinal;

            if (ordinal >= exp_dir->NumberOfFunctions) {
                continue;
            }

            if (SND_MUL_OVERFLOWS_DWORD(ordinal, sizeof(DWORD))) {
                continue;
            }
            DWORD func_offset = (DWORD)ordinal * (DWORD)sizeof(DWORD);
            if (SND_ADD_OVERFLOWS_DWORD(exp_dir->AddressOfFunctions, func_offset)) {
                continue;
            }

            DWORD *p_func_rva =
                (DWORD *)snd_pe_rva_to_ptr(parser, exp_dir->AddressOfFunctions + func_offset, sizeof(DWORD));
            if (!p_func_rva) {
                continue;
            }
            func_rva = *p_func_rva;
            break;
        }
    }

    if (func_rva == 0) {
        return SND_ERR(SND_STATUS_EXPORT_SYMBOL_MISSING);
    }

    if (SND_IN_BOUNDS(func_rva, export_dir.VirtualAddress, export_dir.Size)) {
        if (resolver == NULL) {
            return SND_ERR_CTX(SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED,
                               "Export forwarder encountered but no module resolver callback was provided");
        }

        if (depth >= SND_FWD_MAX_DEPTH) {
            return SND_ERR_CTX(SND_STATUS_EXPORT_FORWARDER_EXCEEDED,
                               "Export forwarder loop depth exceeded maximum (%d)", SND_FWD_MAX_DEPTH);
        }

        const char *fwd_str = (char *)snd_pe_rva_to_ptr(parser, func_rva, 1);
        if (!fwd_str) {
            return SND_ERR_CTX(SND_STATUS_EXPORT_FORWARDER_INVALID, "Forwarder string RVA outside image bounds");
        }
        SIZE_T max_fwd = (export_dir.VirtualAddress + export_dir.Size) - func_rva;

        SIZE_T fwd_offset    = SND_PTR_DIFF(fwd_str, parser->source.data);
        SIZE_T max_available = parser->source.size - fwd_offset;

        max_fwd = SND_MIN(max_fwd, max_available);

        if (snd_strnlen(fwd_str, max_fwd) >= max_fwd) {
            return SND_ERR_CTX(SND_STATUS_EXPORT_FORWARDER_INVALID, "Forwarder string unterminated or exceeds bounds");
        }

        const char *dot = snd_strnchr(fwd_str, '.', max_fwd);
        if (!dot) {
            return SND_ERR_CTX(SND_STATUS_EXPORT_FORWARDER_INVALID, "Forwarder string missing dot delimiter");
        }

        SIZE_T pfx_len               = SND_PTR_DIFF(dot, fwd_str);
        char   fwd_dll[SND_MAX_PATH] = {0};
        if (pfx_len + 5 >= SND_MAX_PATH) {
            return SND_ERR_CTX(SND_STATUS_EXPORT_FORWARDER_INVALID, "Forwarder DLL name exceeds SND_MAX_PATH");
        }

        snd_strncpy(fwd_dll, SND_MAX_PATH, fwd_str, pfx_len);
        if (pfx_len < 4 || snd_strnicmp(fwd_dll + pfx_len - 4, ".dll", 4) != 0) {
            snd_strncat(fwd_dll, SND_MAX_PATH, ".dll", 4);
        }

        wchar_t wfwd_dll[SND_MAX_PATH];
        snd_ascii_to_wide(wfwd_dll, SND_MAX_PATH, fwd_dll, SND_MAX_PATH);

        PVOID fwd_base = NULL;
        SND_TRY(resolver(wfwd_dll, &fwd_base));

        snd_buffer_t    fwd_buf = {.data = fwd_base, .size = SND_SYS_DLL_SIZE_DEFAULT};
        snd_pe_parser_t fwd_parser;

        SND_TRY(snd_pe_parse(&fwd_buf, TRUE, &fwd_parser));

        SIZE_T fwd_image_size = SND_PE_GET_NT_FIELD(&fwd_parser, OptionalHeader.SizeOfImage);
        if (fwd_image_size > 0) {
            fwd_parser.source.size = fwd_image_size;
        }

        const char *fwd_func = dot + 1;

        if (*fwd_func == '#') {
            uint32_t ord = 0;
            if (!snd_atou32_bounded(fwd_func + 1, 6, &ord) || ord == 0 || ord > SND_MAX_WORD) {
                return SND_ERR(SND_STATUS_EXPORT_ORDINAL_INVALID);
            }

            return pe_get_export_address_impl(&fwd_parser, (char *)(ULONG_PTR)ord, 0, func_addr_out, resolver, FALSE,
                                              depth + 1);
        }

        if (is_hash_mode) {
            DWORD fwd_func_hash = snd_hash(fwd_func);
            return pe_get_export_address_impl(&fwd_parser, NULL, fwd_func_hash, func_addr_out, resolver, TRUE,
                                              depth + 1);
        } else {
            return pe_get_export_address_impl(&fwd_parser, fwd_func, 0, func_addr_out, resolver, FALSE, depth + 1);
        }
    }

    PVOID final_ptr = snd_pe_rva_to_ptr(parser, func_rva, 1);
    if (!final_ptr) {
        return SND_ERR(SND_STATUS_EXPORT_DIRECTORY_INVALID);
    }

    *func_addr_out = (FARPROC)final_ptr;
    return SND_OK;
}

snd_status_t snd_pe_get_export_address(const snd_pe_parser_t *parser, const char *func_name, FARPROC *func_addr_out,
                                       snd_module_resolver_cb resolver) {
    return pe_get_export_address_impl(parser, func_name, 0, func_addr_out, resolver, FALSE, 0);
}

snd_status_t snd_pe_get_export_address_hash(const snd_pe_parser_t *parser, DWORD func_hash, FARPROC *func_addr_out,
                                            snd_module_resolver_cb resolver) {
    return pe_get_export_address_impl(parser, NULL, func_hash, func_addr_out, resolver, TRUE, 0);
}

snd_status_t snd_pe_enumerate_exports(const snd_pe_parser_t *parser, snd_pe_export_enum_cb callback, PVOID user_ctx) {
    SND_CHECK_NULL(parser, callback);

    SND_IMAGE_DATA_DIRECTORY export_dir = {0};
    SND_TRY(snd_pe_get_directory(parser, SND_IMAGE_DIRECTORY_ENTRY_EXPORT, &export_dir));

    PSND_IMAGE_EXPORT_DIRECTORY exp_dir = (PSND_IMAGE_EXPORT_DIRECTORY)snd_pe_rva_to_ptr(
        parser, export_dir.VirtualAddress, sizeof(SND_IMAGE_EXPORT_DIRECTORY));
    if (!exp_dir) {
        return SND_ERR(SND_STATUS_EXPORT_DIRECTORY_NULL);
    }
    if (exp_dir->NumberOfFunctions == 0) {
        return SND_OK;
    }

    DWORD *names = (DWORD *)snd_pe_rva_to_ptr(parser, exp_dir->AddressOfNames, exp_dir->NumberOfNames * sizeof(DWORD));
    DWORD *funcs =
        (DWORD *)snd_pe_rva_to_ptr(parser, exp_dir->AddressOfFunctions, exp_dir->NumberOfFunctions * sizeof(DWORD));
    WORD *ords =
        (WORD *)snd_pe_rva_to_ptr(parser, exp_dir->AddressOfNameOrdinals, exp_dir->NumberOfNames * sizeof(WORD));

    if (!funcs) {
        return SND_ERR(SND_STATUS_EXPORT_DIRECTORY_INVALID);
    }

    for (DWORD i = 0; i < exp_dir->NumberOfNames; i++) {
        if (!names || !ords) {
            break;
        }

        const char *func_name = (const char *)snd_pe_rva_to_ptr(parser, names[i], 1);
        WORD        ord_val   = ords[i];

        if (ord_val >= exp_dir->NumberOfFunctions) {
            continue;
        }

        DWORD func_rva = funcs[ord_val];
        if (!func_rva) {
            continue;
        }

        PVOID func_addr = snd_pe_rva_to_ptr(parser, func_rva, 1);
        if (!func_addr) {
            continue;
        }

        if (!callback(func_name, (WORD)(exp_dir->Base + ord_val), func_addr, user_ctx)) {
            break;
        }
    }

    return SND_OK;
}
