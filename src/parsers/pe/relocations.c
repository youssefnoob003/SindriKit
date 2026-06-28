#include <sindri/common/macros.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/parsers/pe/relocations.h>
#include <sindri/parsers/pe/utils.h>
#include <windows.h>

snd_status_t snd_pe_apply_relocations(PVOID base_address, LONG_PTR delta_offset, const snd_pe_parser_t *parser) {
    if (base_address == NULL) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    if (!parser || !parser->is_mapped) {
        return SND_ERR_CTX(SND_STATUS_IMAGE_NOT_MAPPED, "Relocations can only be applied to mapped images.");
    }

    if (delta_offset == 0) {
        return SND_OK;
    }

    IMAGE_DATA_DIRECTORY reloc_dir = {0};
    snd_pe_get_directory(parser, IMAGE_DIRECTORY_ENTRY_BASERELOC, &reloc_dir);

    BOOL relocs_stripped = (SND_PE_GET_NT_FIELD(parser, FileHeader.Characteristics) & IMAGE_FILE_RELOCS_STRIPPED) != 0;

    if (reloc_dir.VirtualAddress == 0 || reloc_dir.Size == 0 || relocs_stripped) {
        return SND_ERR_CTX(SND_STATUS_RELOCATION_DIRECTORY_INVALID,
                           "Payload cannot be relocated (no relocations or RELOCS_STRIPPED) but "
                           "was mapped at a dynamic base address.");
    }

    if (reloc_dir.Size < sizeof(IMAGE_BASE_RELOCATION)) {
        return SND_ERR(SND_STATUS_RELOCATION_DIRECTORY_INVALID);
    }

    PIMAGE_BASE_RELOCATION reloc =
        (PIMAGE_BASE_RELOCATION)snd_pe_rva_to_ptr(parser, reloc_dir.VirtualAddress, sizeof(IMAGE_BASE_RELOCATION));
    if (!reloc) {
        return SND_ERR(SND_STATUS_RELOCATION_DIRECTORY_INVALID);
    }

    SIZE_T cursor = reloc_dir.VirtualAddress;
    SIZE_T end    = cursor + reloc_dir.Size;

    while (cursor + sizeof(IMAGE_BASE_RELOCATION) <= end) {
        reloc = (PIMAGE_BASE_RELOCATION)snd_pe_rva_to_ptr(parser, (DWORD)cursor, sizeof(IMAGE_BASE_RELOCATION));
        if (!reloc)
            break;

        if (reloc->SizeOfBlock == 0) {
            break;
        }

        if (reloc->SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION) || reloc->SizeOfBlock > end - cursor) {
            break;
        }

        if (!snd_pe_rva_to_ptr(parser, (DWORD)cursor, reloc->SizeOfBlock)) {
            return SND_ERR(SND_STATUS_RELOCATION_DIRECTORY_INVALID);
        }

        WORD *entries       = (WORD *)SND_PTR_ADD(reloc, sizeof(IMAGE_BASE_RELOCATION));
        DWORD entries_count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);

        for (DWORD i = 0; i < entries_count; i++) {
            WORD   entry        = entries[i];
            WORD   type         = entry >> 12;
            WORD   offset       = entry & 0x0FFF;
            SIZE_T patch_offset = (SIZE_T)reloc->VirtualAddress + (SIZE_T)offset;

            if (type == IMAGE_REL_BASED_ABSOLUTE) {
                continue;
            }

            if (type == IMAGE_REL_BASED_HIGHLOW) {
                if (!snd_pe_rva_to_ptr(parser, (DWORD)patch_offset, sizeof(DWORD))) {
                    return SND_ERR(SND_STATUS_RELOCATION_PATCH_OUT_OF_RANGE);
                }
                *(DWORD *)SND_PTR_ADD(base_address, patch_offset) += (DWORD)delta_offset;
            } else if (type == IMAGE_REL_BASED_DIR64) {
                if (!snd_pe_rva_to_ptr(parser, (DWORD)patch_offset, sizeof(ULONGLONG))) {
                    return SND_ERR(SND_STATUS_RELOCATION_PATCH_OUT_OF_RANGE);
                }
                *(ULONGLONG *)SND_PTR_ADD(base_address, patch_offset) += (ULONGLONG)delta_offset;
            } else {
                return SND_ERR_CTX(SND_STATUS_RELOCATION_TYPE_UNSUPPORTED, "Unsupported relocation type: %d", type);
            }
        }

        cursor += reloc->SizeOfBlock;
    }

    return SND_OK;
}
