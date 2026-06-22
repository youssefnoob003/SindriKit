#include <sindri/common/hash.h>
#include <sindri/common/helpers.h>
#include <sindri/common/status.h>
#include <sindri/parsers/pe/pe_exports.h>
#include <sindri/parsers/pe/pe_parser.h>
#include <sindri/parsers/pe/pe_utils.h>
#include <sindri/primitives/modules.h>
#include <windows.h>

static snd_status_t pe_get_export_address_impl(PVOID base_address, SIZE_T size, const char *func_name, DWORD func_hash,
                                               FARPROC *func_addr_out, PVOID resolver, BOOL is_hash_mode, int depth) {
    if (base_address == NULL || func_addr_out == NULL) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }
    if (!is_hash_mode && func_name == NULL) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }
    if (is_hash_mode && func_hash == 0) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }

    *func_addr_out = NULL;

    snd_buffer_t local_buf;
    local_buf.data = (LPVOID)base_address;
    local_buf.size = size;

    snd_pe_parser_t local_parser;
    snd_status_t    parse_status = snd_pe_parse(&local_buf, TRUE, &local_parser);
    if (parse_status.code != SND_SUCCESS) {
        return parse_status;
    }

    SIZE_T image_size = SND_PE_GET_NT_FIELD(&local_parser, OptionalHeader.SizeOfImage);
    if (size == SND_SYS_DLL_SIZE_DEFAULT && image_size > size) {
        local_parser.source.size = image_size;
    } else if (image_size < local_parser.source.size) {
        local_parser.source.size = image_size;
    }

    IMAGE_DATA_DIRECTORY export_dir = {0};
    snd_pe_get_directory(&local_parser, IMAGE_DIRECTORY_ENTRY_EXPORT, &export_dir);

    if (export_dir.VirtualAddress == 0) {
        return SND_ERR_CTX(SND_STATUS_EXPORT_NOT_FOUND, "Module has no export directory.");
    }

    PIMAGE_EXPORT_DIRECTORY exp_dir = (PIMAGE_EXPORT_DIRECTORY)snd_pe_rva_to_ptr(
        &local_parser, export_dir.VirtualAddress, sizeof(IMAGE_EXPORT_DIRECTORY));
    if (!exp_dir) {
        return SND_ERR_CTX(SND_STATUS_EXPORT_DIRECTORY_INVALID, "Invalid export directory memory bounds.");
    }
    if (exp_dir->NumberOfFunctions == 0) {
        return SND_ERR_CTX(SND_STATUS_EXPORT_NOT_FOUND, "Export table is empty.");
    }

    DWORD func_rva          = 0;
    BOOL  is_ordinal_lookup = (!is_hash_mode && func_name != NULL && IS_INTRESOURCE(func_name));

    if (is_ordinal_lookup) {
        ULONG_PTR requested_ordinal = (ULONG_PTR)func_name;
        ULONG_PTR base_ord          = exp_dir->Base;

        if (requested_ordinal < base_ord || requested_ordinal >= base_ord + exp_dir->NumberOfFunctions) {
            return SND_ERR_CTX(SND_STATUS_EXPORT_NOT_FOUND, "Export ordinal out of range.");
        }

        DWORD  func_idx   = (DWORD)(requested_ordinal - base_ord);
        DWORD *p_func_rva = (DWORD *)snd_pe_rva_to_ptr(
            &local_parser, exp_dir->AddressOfFunctions + func_idx * sizeof(DWORD), sizeof(DWORD));
        if (!p_func_rva) {
            return SND_ERR_CTX(SND_STATUS_EXPORT_NOT_FOUND, "Export ordinal points to invalid memory.");
        }
        func_rva = *p_func_rva;
    } else {
        for (DWORD i = 0; i < exp_dir->NumberOfNames; i++) {
            DWORD *p_name_rva =
                (DWORD *)snd_pe_rva_to_ptr(&local_parser, exp_dir->AddressOfNames + i * sizeof(DWORD), sizeof(DWORD));
            if (!p_name_rva)
                break;
            DWORD name_rva = *p_name_rva;

            const char *current_name = (const char *)snd_pe_rva_to_ptr(&local_parser, name_rva, 1);
            if (!current_name)
                continue;

            SIZE_T name_offset     = (SIZE_T)((BYTE *)current_name - (BYTE *)local_parser.source.data);
            SIZE_T max_safe_length = local_parser.source.size - name_offset;

            if (snd_strnlen(current_name, max_safe_length) == max_safe_length)
                continue;

            if (!is_hash_mode) {
                if (snd_strncmp(current_name, func_name, max_safe_length) != 0)
                    continue;
            } else {
                if (snd_hash(current_name) != func_hash)
                    continue;
            }

            WORD *p_ordinal = (WORD *)snd_pe_rva_to_ptr(
                &local_parser, exp_dir->AddressOfNameOrdinals + i * sizeof(WORD), sizeof(WORD));
            if (!p_ordinal) {
                break;
            }
            WORD ordinal = *p_ordinal;

            if (ordinal >= exp_dir->NumberOfFunctions) {
                continue;
            }

            DWORD *p_func_rva = (DWORD *)snd_pe_rva_to_ptr(
                &local_parser, exp_dir->AddressOfFunctions + ordinal * sizeof(DWORD), sizeof(DWORD));
            if (!p_func_rva) {
                continue;
            }
            func_rva = *p_func_rva;
            break;
        }
    }

    if (func_rva == 0) {
        return SND_ERR(SND_STATUS_EXPORT_NOT_FOUND);
    }

    if (func_rva >= export_dir.VirtualAddress && func_rva < export_dir.VirtualAddress + export_dir.Size) {
        if (resolver == NULL) {
            return SND_ERR_CTX(SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED, "Resolver is NULL.");
        }

        if (depth >= SND_FWD_MAX_DEPTH) {
            return SND_ERR_CTX(SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED, "Max export forwarder loop depth exceeded.");
        }

        const char *fwd_str = (const char *)snd_pe_rva_to_ptr(&local_parser, func_rva, 1);
        if (!fwd_str)
            return SND_ERR_CTX(SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED, "Malformed forwarder string.");
        SIZE_T max_fwd = (export_dir.VirtualAddress + export_dir.Size) - func_rva;

        SIZE_T fwd_offset    = (SIZE_T)((BYTE *)fwd_str - (BYTE *)local_parser.source.data);
        SIZE_T max_available = local_parser.source.size - fwd_offset;

        if (max_fwd > max_available) {
            max_fwd = max_available;
        }
        if (snd_strnlen(fwd_str, max_fwd) >= max_fwd) {
            return SND_ERR_CTX(SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED, "Malformed forwarder string.");
        }

        const char *dot = snd_strnchr(fwd_str, '.', max_fwd);
        if (!dot)
            return SND_ERR_CTX(SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED, "Forwarder string missing dot separator.");

        SIZE_T pfx_len = (SIZE_T)(dot - fwd_str);
        char   fwd_dll[MAX_PATH];
        if (pfx_len + 5 >= MAX_PATH)
            return SND_ERR_CTX(SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED, "Forwarder DLL path exceeds maximum length.");

        for (SIZE_T i = 0; i < pfx_len; i++) {
            fwd_dll[i] = fwd_str[i];
        }
        fwd_dll[pfx_len] = '\0';
        snd_strncat(fwd_dll, MAX_PATH, ".dll", 4);

        PVOID       fwd_base = NULL;
        const char *fwd_func = dot + 1;

        if (is_hash_mode) {
            DWORD                       dll_hash      = snd_hash_lower(fwd_dll);
            snd_module_resolver_hash_cb hash_resolver = (snd_module_resolver_hash_cb)resolver;

            snd_status_t hash_res_status = hash_resolver(dll_hash, &fwd_base);
            if (hash_res_status.code != SND_SUCCESS || !fwd_base)
                return SND_ERR(SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED);

            if (*fwd_func == '#') {
                ULONG_PTR ord = 0;
                for (const char *p = fwd_func + 1; *p >= '0' && *p <= '9'; p++) {
                    ord = ord * 10 + (*p - '0');
                }
                return pe_get_export_address_impl(fwd_base, SND_SYS_DLL_SIZE_DEFAULT, (const char *)ord, 0,
                                                  func_addr_out, resolver, FALSE, depth + 1);
            }

            DWORD fwd_func_hash = snd_hash(fwd_func);
            return pe_get_export_address_impl(fwd_base, SND_SYS_DLL_SIZE_DEFAULT, NULL, fwd_func_hash, func_addr_out,
                                              resolver, TRUE, depth + 1);

        } else {
            wchar_t wfwd_dll[MAX_PATH];
            for (SIZE_T k = 0; k <= pfx_len + 4; k++) {
                wfwd_dll[k] = (wchar_t)(unsigned char)fwd_dll[k];
            }

            snd_module_resolver_cb string_resolver = (snd_module_resolver_cb)resolver;
            snd_status_t           str_res_status  = string_resolver(wfwd_dll, &fwd_base);
            if (str_res_status.code != SND_SUCCESS)
                return str_res_status;

            if (*fwd_func == '#') {
                ULONG_PTR ord = 0;
                for (const char *p = fwd_func + 1; *p >= '0' && *p <= '9'; p++) {
                    ord = ord * 10 + (*p - '0');
                }
                return pe_get_export_address_impl(fwd_base, SND_SYS_DLL_SIZE_DEFAULT, (const char *)ord, 0,
                                                  func_addr_out, resolver, FALSE, depth + 1);
            }

            return pe_get_export_address_impl(fwd_base, SND_SYS_DLL_SIZE_DEFAULT, fwd_func, 0, func_addr_out, resolver,
                                              FALSE, depth + 1);
        }
    }

    PVOID final_ptr = snd_pe_rva_to_ptr(&local_parser, func_rva, 1);
    if (!final_ptr) {
        return SND_ERR_CTX(SND_STATUS_EXPORT_DIRECTORY_INVALID, "Export RVA points outside valid image boundaries.");
    }

    *func_addr_out = (FARPROC)final_ptr;
    return SND_OK;
}

snd_status_t snd_pe_get_export_address(PVOID base_address, SIZE_T size, const char *func_name, FARPROC *func_addr_out,
                                       snd_module_resolver_cb resolver) {
    return pe_get_export_address_impl(base_address, size, func_name, 0, func_addr_out, (PVOID)resolver, FALSE, 0);
}

snd_status_t snd_pe_get_export_address_by_hash(PVOID base_address, SIZE_T size, DWORD func_hash, FARPROC *func_addr_out,
                                               snd_module_resolver_hash_cb resolver) {
    return pe_get_export_address_impl(base_address, size, NULL, func_hash, func_addr_out, (PVOID)resolver, TRUE, 0);
}
