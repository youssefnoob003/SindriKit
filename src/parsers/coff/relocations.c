#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/parsers/coff/parser.h>
#include <sindri/parsers/coff/relocations.h>
#include <sindri/parsers/coff/symbols.h>

snd_status_t snd_coff_get_relocations(const snd_coff_parser_t *parser, const IMAGE_SECTION_HEADER *section,
                                      PIMAGE_RELOCATION *relocations_out, DWORD *count_out) {
    if (!parser || !section || !relocations_out || !count_out) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    if (section->NumberOfRelocations == 0 || section->PointerToRelocations == 0) {
        *relocations_out = NULL;
        *count_out       = 0;
        return SND_OK;
    }

    DWORD num_relocs   = section->NumberOfRelocations;
    DWORD reloc_offset = section->PointerToRelocations;
    BOOL  is_extended  = FALSE;

    if (num_relocs == 0xFFFF) {
        if (!snd_buffer_bounds_check(&parser->source, reloc_offset, sizeof(IMAGE_RELOCATION))) {
            return SND_ERR(SND_STATUS_NT_HEADERS_TRUNCATED);
        }

        PIMAGE_RELOCATION first_reloc = (PIMAGE_RELOCATION)SND_PTR_ADD(parser->source.data, reloc_offset);

        num_relocs  = first_reloc->VirtualAddress;
        is_extended = TRUE;

        if (num_relocs == 0) {
            return SND_ERR(SND_STATUS_NT_HEADERS_TRUNCATED);
        }
    }

    if (num_relocs > 0 && (((SIZE_T)-1) / sizeof(IMAGE_RELOCATION)) < num_relocs) {
        return SND_ERR(SND_STATUS_UNSUPPORTED);
    }

    SIZE_T expected_size = (SIZE_T)num_relocs * sizeof(IMAGE_RELOCATION);

    if ((SIZE_T)reloc_offset + expected_size < (SIZE_T)reloc_offset) {
        return SND_ERR(SND_STATUS_INTEGER_OVERFLOW);
    }

    if (!snd_buffer_bounds_check(&parser->source, reloc_offset, expected_size)) {
        return SND_ERR(SND_STATUS_NT_HEADERS_TRUNCATED);
    }

    if (is_extended) {
        *relocations_out = (PIMAGE_RELOCATION)SND_PTR_ADD(parser->source.data, reloc_offset + sizeof(IMAGE_RELOCATION));
        *count_out       = (num_relocs > 0) ? (num_relocs - 1) : 0;
    } else {
        *relocations_out = (PIMAGE_RELOCATION)SND_PTR_ADD(parser->source.data, reloc_offset);
        *count_out       = num_relocs;
    }

    return SND_OK;
}

snd_status_t snd_coff_apply_relocations(const snd_coff_parser_t *parser, LPVOID *section_map, LPVOID *symbol_map,
                                        LPVOID execution_base) {
    if (!parser || !section_map || !symbol_map) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    for (DWORD i = 0; i < parser->sections_count; i++) {
        PIMAGE_SECTION_HEADER sec = &parser->section_head[i];

        PIMAGE_RELOCATION relocs      = NULL;
        DWORD             reloc_count = 0;

        snd_status_t reloc_status = snd_coff_get_relocations(parser, sec, &relocs, &reloc_count);
        if (SND_FAILED(reloc_status)) {
            return reloc_status;
        }

        if (reloc_count == 0) {
            continue;
        }

        LPVOID section_dest = section_map[i + 1];
        if (!section_dest) {
            continue;
        }

        for (DWORD j = 0; j < reloc_count; j++) {
            PIMAGE_RELOCATION rel = &relocs[j];

            if (rel->SymbolTableIndex >= parser->symbol_count) {
                return SND_ERR(SND_STATUS_COFF_SYMBOL_NOT_FOUND);
            }

            PIMAGE_SYMBOL sym = snd_coff_get_symbol_by_index(parser, rel->SymbolTableIndex);
            if (!sym) {
                continue;
            }

            ULONG_PTR target_addr = (ULONG_PTR)symbol_map[rel->SymbolTableIndex];
            if (target_addr == 0 && sym->SectionNumber == IMAGE_SYM_UNDEFINED) {
                return SND_ERR(SND_STATUS_COFF_SYMBOL_RESOLUTION_FAILED);
            }

            SIZE_T reloc_size = 4;
            if (parser->is_64bit && rel->Type == IMAGE_REL_AMD64_ADDR64) {
                reloc_size = 8;
            }

            if (rel->VirtualAddress > sec->SizeOfRawData || reloc_size > (sec->SizeOfRawData - rel->VirtualAddress)) {
                return SND_ERR(SND_STATUS_COFF_RELOCATION_UNSUPPORTED);
            }

            ULONG_PTR reloc_target = (ULONG_PTR)section_dest + rel->VirtualAddress;
            if (parser->is_64bit) {
                switch (rel->Type) {
                case IMAGE_REL_AMD64_ADDR64: // 0x0001
                    *(ULONG64 *)reloc_target += target_addr;
                    break;
                case IMAGE_REL_AMD64_ADDR32NB: // 0x0003
                    *(DWORD *)reloc_target += (DWORD)(target_addr - (ULONG_PTR)execution_base);
                    break;
                case IMAGE_REL_AMD64_REL32: // 0x0004
                    *(LONG32 *)reloc_target += (LONG32)(target_addr - (reloc_target + 4));
                    break;
                case IMAGE_REL_AMD64_REL32_1: // 0x0005
                    *(LONG32 *)reloc_target += (LONG32)(target_addr - (reloc_target + 5));
                    break;
                case IMAGE_REL_AMD64_REL32_2: // 0x0006
                    *(LONG32 *)reloc_target += (LONG32)(target_addr - (reloc_target + 6));
                    break;
                case IMAGE_REL_AMD64_REL32_3: // 0x0007
                    *(LONG32 *)reloc_target += (LONG32)(target_addr - (reloc_target + 7));
                    break;
                case IMAGE_REL_AMD64_REL32_4: // 0x0008
                    *(LONG32 *)reloc_target += (LONG32)(target_addr - (reloc_target + 8));
                    break;
                case IMAGE_REL_AMD64_REL32_5: // 0x0009
                    *(LONG32 *)reloc_target += (LONG32)(target_addr - (reloc_target + 9));
                    break;
                default:
                    break;
                }
            } else {
                switch (rel->Type) {
                case IMAGE_REL_I386_DIR32: // 0x0006
                    *(DWORD *)reloc_target += (DWORD)target_addr;
                    break;
                case IMAGE_REL_I386_DIR32NB: // 0x0007
                    *(DWORD *)reloc_target += (DWORD)(target_addr - (ULONG_PTR)execution_base);
                    break;
                case IMAGE_REL_I386_REL32: // 0x0014
                    *(LONG32 *)reloc_target += (LONG32)(target_addr - (reloc_target + 4));
                    break;
                default:
                    break;
                }
            }
        }
    }
    return SND_OK;
}
