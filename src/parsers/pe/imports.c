#include <sindri/common/memory.h>
#include <sindri/common/string.h>
#include <sindri/internal/windows/pe.h>
#include <sindri/parsers/pe/imports.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/parsers/pe/status.h>
#include <sindri/parsers/pe/utils.h>

snd_status_t snd_pe_get_import_descriptor(const snd_pe_parser_t *parser, DWORD index,
                                          const SND_IMAGE_IMPORT_DESCRIPTOR **out_desc) {
    SND_CHECK_NULL(parser, out_desc);

    *out_desc = NULL;

    if (parser->imports_rva == 0) {
        return SND_OK;
    }

    SIZE_T offset = (SIZE_T)index * sizeof(SND_IMAGE_IMPORT_DESCRIPTOR);
    if (offset > SND_MAX_DWORD || SND_ADD_OVERFLOWS_DWORD(parser->imports_rva, offset)) {
        return SND_ERR(SND_STATUS_IMPORT_DESCRIPTOR_OVERFLOW);
    }

    DWORD desc_rva = parser->imports_rva + (DWORD)offset;
    if (parser->import_size > 0) {
        if (SND_ADD_OVERFLOWS_DWORD(parser->imports_rva, parser->import_size)) {
            return SND_ERR(SND_STATUS_IMPORT_DESCRIPTOR_OVERFLOW);
        }
        if (desc_rva + sizeof(SND_IMAGE_IMPORT_DESCRIPTOR) > parser->imports_rva + parser->import_size) {
            return SND_OK;
        }
    }

    const SND_IMAGE_IMPORT_DESCRIPTOR *imp =
        (const SND_IMAGE_IMPORT_DESCRIPTOR *)snd_pe_rva_to_ptr(parser, desc_rva, sizeof(SND_IMAGE_IMPORT_DESCRIPTOR));
    if (!imp)
        return SND_ERR(SND_STATUS_IMPORT_DESCRIPTOR_INVALID);

    if (imp->Name == 0 && imp->FirstThunk == 0)
        return SND_OK;

    *out_desc = imp;
    return SND_OK;
}

snd_status_t snd_pe_get_import_name(const snd_pe_parser_t *parser, const SND_IMAGE_IMPORT_DESCRIPTOR *desc,
                                    const char **out_name) {
    SND_CHECK_NULL(parser, desc, out_name);

    if (desc->Name == 0)
        return SND_ERR(SND_STATUS_IMPORT_NAME_INVALID);

    const char *dll_name = (const char *)snd_pe_rva_to_ptr(parser, desc->Name, 1);
    if (!dll_name) {
        return SND_ERR_CTX(SND_STATUS_IMPORT_NAME_INVALID, "Import DLL name RVA 0x%08X outside image bounds",
                           desc->Name);
    }

    SIZE_T dll_p_offset     = SND_PTR_DIFF(dll_name, parser->source.data);
    SIZE_T max_dll_name_len = parser->source.size - dll_p_offset;
    if (snd_strnlen(dll_name, max_dll_name_len) == max_dll_name_len) {
        return SND_ERR_CTX(SND_STATUS_IMPORT_NAME_INVALID, "Import DLL name string unterminated or overflows buffer");
    }

    *out_name = dll_name;
    return SND_OK;
}

snd_status_t snd_pe_get_import_thunk(const snd_pe_parser_t *parser, const SND_IMAGE_IMPORT_DESCRIPTOR *desc,
                                     DWORD thunk_index, snd_pe_import_thunk_t *out_thunk) {
    SND_CHECK_NULL(parser, desc, out_thunk);
    snd_memzero(out_thunk, sizeof(*out_thunk));

    DWORD lookup_rva = desc->u.OriginalFirstThunk ? desc->u.OriginalFirstThunk : desc->FirstThunk;
    DWORD first_rva  = desc->FirstThunk;

    if (lookup_rva == 0 || first_rva == 0)
        return SND_OK;

    SIZE_T thunk_stride = parser->is_64bit ? sizeof(SND_IMAGE_THUNK_DATA64) : sizeof(SND_IMAGE_THUNK_DATA32);
    SIZE_T offset       = (SIZE_T)thunk_index * thunk_stride;

    if (offset > SND_MAX_DWORD || SND_ADD_OVERFLOWS_DWORD(lookup_rva, offset) ||
        SND_ADD_OVERFLOWS_DWORD(first_rva, (DWORD)offset)) {
        return SND_ERR(SND_STATUS_IMPORT_THUNK_OVERFLOW);
    }

    DWORD org_thunk_rva   = lookup_rva + (DWORD)offset;
    DWORD first_thunk_rva = first_rva + (DWORD)offset;

    PVOID org_thunk_ptr   = snd_pe_rva_to_ptr(parser, org_thunk_rva, thunk_stride);
    PVOID first_thunk_ptr = snd_pe_rva_to_ptr(parser, first_thunk_rva, thunk_stride);

    if (!org_thunk_ptr || !first_thunk_ptr) {
        return SND_ERR(SND_STATUS_IMPORT_THUNK_OUT_OF_BOUNDS);
    }

    ULONGLONG thunk_val  = 0;
    BOOL      is_ordinal = FALSE;

    if (parser->is_64bit) {
        thunk_val  = ((PSND_IMAGE_THUNK_DATA64)org_thunk_ptr)->u1.AddressOfData;
        is_ordinal = SND_PE_SNAP_BY_ORDINAL64(thunk_val) ? TRUE : FALSE;
    } else {
        thunk_val  = ((PSND_IMAGE_THUNK_DATA32)org_thunk_ptr)->u1.AddressOfData;
        is_ordinal = SND_PE_SNAP_BY_ORDINAL32(thunk_val) ? TRUE : FALSE;
    }

    if (thunk_val == 0)
        return SND_OK;

    out_thunk->iat_slot   = first_thunk_ptr;
    out_thunk->is_ordinal = is_ordinal;

    if (is_ordinal) {
        out_thunk->ordinal = (WORD)SND_PE_ORDINAL(thunk_val);
    } else {
        if (thunk_val > SND_MAX_DWORD)
            return SND_ERR(SND_STATUS_IMPORT_THUNK_OVERFLOW);

        PSND_IMAGE_IMPORT_BY_NAME func =
            (PSND_IMAGE_IMPORT_BY_NAME)snd_pe_rva_to_ptr(parser, (DWORD)thunk_val, sizeof(SND_IMAGE_IMPORT_BY_NAME));

        if (!func) {
            if (desc->u.OriginalFirstThunk == 0)
                return SND_OK;
            return SND_ERR(SND_STATUS_IMPORT_THUNK_INVALID);
        }

        SIZE_T func_p_offset     = SND_PTR_DIFF(func->Name, parser->source.data);
        SIZE_T max_func_name_len = parser->source.size - func_p_offset;

        if (snd_strnlen((char *)func->Name, max_func_name_len) == max_func_name_len) {
            return SND_ERR(SND_STATUS_IMPORT_NAME_INVALID);
        }

        out_thunk->name = (const char *)func->Name;
    }

    return SND_OK;
}
