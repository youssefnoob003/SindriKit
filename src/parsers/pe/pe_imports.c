#include "sindri/common/status.h"

#include <sindri/common/helpers.h>
#include <sindri/parsers/pe/pe_imports.h>
#include <sindri/parsers/pe/pe_parser.h>
#include <sindri/parsers/pe/pe_utils.h>
#include <stddef.h>
#include <windows.h>

snd_status_t snd_pe_resolve_imports(PVOID base_address, const snd_module_api_t *mod_api,
                                    const snd_pe_parser_t *parser) {
    if (!base_address || !mod_api || !mod_api->load_library || !mod_api->get_proc_address || !parser) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }

    if (parser->imports_rva == 0) {
        return SND_OK;
    }

    if (!parser->is_mapped) {
        return SND_ERR_CTX(SND_STATUS_INVALID_PARAMETER, "Imports can only be resolved on mapped images.");
    }

    DWORD  current_descriptor_rva = parser->imports_rva;
    SIZE_T thunk_stride           = parser->is_64bit ? sizeof(IMAGE_THUNK_DATA64) : sizeof(IMAGE_THUNK_DATA32);

    while (1) {
        PIMAGE_IMPORT_DESCRIPTOR imp = (PIMAGE_IMPORT_DESCRIPTOR)snd_pe_rva_to_ptr(parser, current_descriptor_rva,
                                                                                   sizeof(IMAGE_IMPORT_DESCRIPTOR));
        if (!imp) {
            return SND_ERR(SND_STATUS_IMPORT_DIRECTORY_INVALID);
        }

        if (imp->Name == 0 && imp->FirstThunk == 0) {
            break;
        }

        if (imp->Name == 0) {
            current_descriptor_rva += sizeof(IMAGE_IMPORT_DESCRIPTOR);
            continue;
        }

        char *dll_name = (char *)snd_pe_rva_to_ptr(parser, imp->Name, 1);
        if (!dll_name) {
            return SND_ERR(SND_STATUS_IMPORT_DIRECTORY_INVALID);
        }

        SIZE_T dll_p_offset     = (SIZE_T)((BYTE *)dll_name - (BYTE *)parser->source.data);
        size_t max_dll_name_len = parser->source.size - dll_p_offset;
        if (snd_strnlen(dll_name, max_dll_name_len) == max_dll_name_len) {
            return SND_ERR(SND_STATUS_IMPORT_DIRECTORY_INVALID);
        }

        HMODULE      h_dll       = NULL;
        snd_status_t load_status = mod_api->load_library(dll_name, &h_dll);
        if (load_status.code != SND_SUCCESS || !h_dll) {
            if (!snd_pe_compatibility_check(parser->is_64bit)) {
                return SND_ERR_CTX(SND_STATUS_IMPORT_DLL_LOAD_FAILED,
                                   "Failed to load imported DLL: '%s' (If using winapi, arch mismatch "
                                   "is the culprit, try native strategy instead)",
                                   dll_name);
            }
            return SND_ERR_CTX(SND_STATUS_IMPORT_DLL_LOAD_FAILED, "Failed to load imported DLL: '%s'", dll_name);
        }

        DWORD original_thunk_rva = imp->OriginalFirstThunk ? imp->OriginalFirstThunk : imp->FirstThunk;
        if (original_thunk_rva == 0 || imp->FirstThunk == 0) {
            current_descriptor_rva += sizeof(IMAGE_IMPORT_DESCRIPTOR);
            continue;
        }

        SIZE_T max_thunks = (SIZE_T)-1;

        SIZE_T resolved = 0;
        for (; resolved < max_thunks; resolved++) {
            SIZE_T current_offset = resolved * thunk_stride;
            DWORD  org_rva        = original_thunk_rva + (DWORD)current_offset;
            DWORD  first_rva      = imp->FirstThunk + (DWORD)current_offset;

            PVOID org_thunk_ptr   = snd_pe_rva_to_ptr(parser, org_rva, thunk_stride);
            PVOID first_thunk_ptr = snd_pe_rva_to_ptr(parser, first_rva, thunk_stride);

            if (!org_thunk_ptr || !first_thunk_ptr) {
                return SND_ERR(SND_STATUS_IMPORT_THUNK_INVALID);
            }

            ULONGLONG thunk_val  = 0;
            BOOL      is_ordinal = FALSE;

            if (parser->is_64bit) {
                thunk_val  = ((PIMAGE_THUNK_DATA64)org_thunk_ptr)->u1.AddressOfData;
                is_ordinal = SND_PE_SNAP_BY_ORDINAL64(thunk_val) ? TRUE : FALSE;
            } else {
                thunk_val  = ((PIMAGE_THUNK_DATA32)org_thunk_ptr)->u1.AddressOfData;
                is_ordinal = SND_PE_SNAP_BY_ORDINAL32(thunk_val) ? TRUE : FALSE;
            }

            if (thunk_val == 0)
                break;

            FARPROC func_addr = NULL;
            if (is_ordinal) {
                ULONG_PTR ordinal = SND_PE_ORDINAL(thunk_val);
                mod_api->get_proc_address(h_dll, (LPCSTR)ordinal, &func_addr);
            } else {
                DWORD import_by_name_rva = (DWORD)thunk_val;

                PIMAGE_IMPORT_BY_NAME func =
                    (PIMAGE_IMPORT_BY_NAME)snd_pe_rva_to_ptr(parser, import_by_name_rva, sizeof(IMAGE_IMPORT_BY_NAME));
                if (!func) {
                    if (imp->OriginalFirstThunk == 0) {
                        break;
                    }
                    return SND_ERR(SND_STATUS_IMPORT_THUNK_INVALID);
                }

                SIZE_T func_p_offset     = (SIZE_T)((BYTE *)func->Name - (BYTE *)parser->source.data);
                size_t max_func_name_len = parser->source.size - func_p_offset;

                if (snd_strnlen((char *)func->Name, max_func_name_len) == max_func_name_len) {
                    return SND_ERR(SND_STATUS_IMPORT_THUNK_INVALID);
                }

                snd_status_t status = mod_api->get_proc_address(h_dll, (LPCSTR)func->Name, &func_addr);
                if (status.code != SND_SUCCESS) {
                    return status;
                }
            }

            if (parser->is_64bit) {
                ((PIMAGE_THUNK_DATA64)first_thunk_ptr)->u1.Function = (ULONGLONG)func_addr;
            } else {
                ((PIMAGE_THUNK_DATA32)first_thunk_ptr)->u1.Function = (DWORD)(ULONG_PTR)func_addr;
            }
        }

        current_descriptor_rva += sizeof(IMAGE_IMPORT_DESCRIPTOR);
    }

    return SND_OK;
}
