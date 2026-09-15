#include <sindri/common/macros.h>
#include <sindri/internal/windows/coff.h>
#include <sindri/parsers/coff/parser.h>
#include <sindri/parsers/coff/relocations.h>
#include <sindri/parsers/coff/status.h>

snd_status_t snd_coff_get_relocation_patch_size(BOOL is_64bit, WORD reloc_type, SIZE_T *size_out) {
    SND_CHECK_NULL(size_out);

    if (is_64bit) {
        switch (reloc_type) {
        case SND_IMAGE_REL_AMD64_ADDR64:
            *size_out = 8;
            return SND_OK;
        case SND_IMAGE_REL_AMD64_ADDR32NB:
        case SND_IMAGE_REL_AMD64_REL32:
        case SND_IMAGE_REL_AMD64_REL32_1:
        case SND_IMAGE_REL_AMD64_REL32_2:
        case SND_IMAGE_REL_AMD64_REL32_3:
        case SND_IMAGE_REL_AMD64_REL32_4:
        case SND_IMAGE_REL_AMD64_REL32_5:
            *size_out = 4;
            return SND_OK;
        default:
            return SND_ERR(SND_STATUS_COFF_RELOCATION_TYPE_UNSUPPORTED);
        }
    } else {
        switch (reloc_type) {
        case SND_IMAGE_REL_I386_DIR32:
        case SND_IMAGE_REL_I386_DIR32NB:
        case SND_IMAGE_REL_I386_REL32:
            *size_out = 4;
            return SND_OK;
        default:
            return SND_ERR(SND_STATUS_COFF_RELOCATION_TYPE_UNSUPPORTED);
        }
    }
}

snd_status_t snd_coff_get_relocations(const snd_coff_parser_t *parser, const SND_IMAGE_SECTION_HEADER *section,
                                      PSND_IMAGE_RELOCATION *relocations_out, DWORD *count_out) {
    SND_CHECK_NULL(parser, section, relocations_out, count_out);

    if (section->NumberOfRelocations == 0) {
        *relocations_out = NULL;
        *count_out       = 0;
        return SND_OK;
    }

    if (section->PointerToRelocations == 0) {
        return SND_ERR(SND_STATUS_RELOCATION_TABLE_MISSING);
    }

    DWORD num_relocs   = section->NumberOfRelocations;
    DWORD reloc_offset = section->PointerToRelocations;
    BOOL  is_extended  = FALSE;

    if (num_relocs == SND_MAX_WORD && (section->Characteristics & SND_IMAGE_SCN_LNK_NRELOC_OVFL)) {
        if (!snd_buffer_bounds_check(&parser->source, reloc_offset, sizeof(SND_IMAGE_RELOCATION))) {
            return SND_ERR(SND_STATUS_RELOCATION_TABLE_TRUNCATED);
        }

        PSND_IMAGE_RELOCATION first_reloc = (PSND_IMAGE_RELOCATION)SND_PTR_ADD(parser->source.data, reloc_offset);
        num_relocs                        = first_reloc->u.RelocCount;
        is_extended                       = TRUE;

        if (num_relocs == 0) {
            return SND_ERR(SND_STATUS_RELOCATION_TABLE_INVALID);
        }
    }

    if (SND_MUL_OVERFLOWS_SIZET(num_relocs, sizeof(SND_IMAGE_RELOCATION))) {
        return SND_ERR(SND_STATUS_RELOCATION_TABLE_OVERFLOW);
    }

    SIZE_T expected_size = (SIZE_T)num_relocs * sizeof(SND_IMAGE_RELOCATION);
    if (!snd_buffer_bounds_check(&parser->source, reloc_offset, expected_size)) {
        return SND_ERR(SND_STATUS_RELOCATION_TABLE_TRUNCATED);
    }

    if (is_extended) {
        *relocations_out =
            (PSND_IMAGE_RELOCATION)SND_PTR_ADD(parser->source.data, reloc_offset + sizeof(SND_IMAGE_RELOCATION));
        *count_out = num_relocs - 1;
    } else {
        *relocations_out = (PSND_IMAGE_RELOCATION)SND_PTR_ADD(parser->source.data, reloc_offset);
        *count_out       = num_relocs;
    }

    return SND_OK;
}
