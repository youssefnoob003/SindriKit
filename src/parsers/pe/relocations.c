#include <sindri/common/memory.h>
#include <sindri/internal/windows/pe.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/parsers/pe/relocations.h>
#include <sindri/parsers/pe/status.h>
#include <sindri/parsers/pe/utils.h>

snd_status_t snd_pe_get_reloc_block(const snd_pe_parser_t *parser, SIZE_T *cursor,
                                    const SND_IMAGE_BASE_RELOCATION **out_block, DWORD *out_entries_count) {
    SND_CHECK_NULL(parser, cursor, out_block, out_entries_count);
    *out_block         = NULL;
    *out_entries_count = 0;

    SND_IMAGE_DATA_DIRECTORY reloc_dir = {0};
    SND_TRY(snd_pe_get_directory(parser, SND_IMAGE_DIRECTORY_ENTRY_BASERELOC, &reloc_dir));

    if (reloc_dir.VirtualAddress == 0 || reloc_dir.Size == 0) {
        return SND_OK;
    }

    if (reloc_dir.Size < sizeof(SND_IMAGE_BASE_RELOCATION)) {
        return SND_ERR_CTX(SND_STATUS_RELOCATION_DIRECTORY_TRUNCATED,
                           "Relocation directory size (%u) smaller than SND_IMAGE_BASE_RELOCATION (%zu)",
                           reloc_dir.Size, sizeof(SND_IMAGE_BASE_RELOCATION));
    }

    if (SND_ADD_OVERFLOWS_DWORD(reloc_dir.VirtualAddress, reloc_dir.Size)) {
        return SND_ERR(SND_STATUS_RELOCATION_DIRECTORY_OVERFLOW);
    }

    SIZE_T dir_start = reloc_dir.VirtualAddress;
    SIZE_T dir_end   = dir_start + reloc_dir.Size;

    if (*cursor == 0) {
        *cursor = dir_start;
    }

    if (*cursor + sizeof(SND_IMAGE_BASE_RELOCATION) > dir_end) {
        return SND_OK;
    }

    const SND_IMAGE_BASE_RELOCATION *reloc =
        (const SND_IMAGE_BASE_RELOCATION *)snd_pe_rva_to_ptr(parser, (DWORD)*cursor, sizeof(SND_IMAGE_BASE_RELOCATION));
    if (!reloc) {
        return SND_ERR(SND_STATUS_RELOCATION_DIRECTORY_OUT_OF_BOUNDS);
    }

    DWORD block_size = reloc->SizeOfBlock;
    if (block_size == 0) {
        return SND_OK;
    }

    if (block_size < sizeof(SND_IMAGE_BASE_RELOCATION) || block_size > (dir_end - *cursor)) {
        return SND_ERR(SND_STATUS_RELOCATION_BLOCK_INVALID);
    }

    if (!snd_pe_rva_to_ptr(parser, (DWORD)*cursor, block_size)) {
        return SND_ERR(SND_STATUS_RELOCATION_DIRECTORY_OUT_OF_BOUNDS);
    }

    *out_block         = reloc;
    *out_entries_count = (block_size - (DWORD)sizeof(SND_IMAGE_BASE_RELOCATION)) / (DWORD)sizeof(WORD);
    *cursor += block_size;

    return SND_OK;
}

snd_status_t snd_pe_get_reloc_entry(const snd_pe_parser_t *parser, const SND_IMAGE_BASE_RELOCATION *block,
                                    DWORD entry_index, snd_pe_reloc_entry_t *out_entry) {
    SND_CHECK_NULL(parser, block, out_entry);
    snd_memzero(out_entry, sizeof(*out_entry));

    DWORD total_entries = (block->SizeOfBlock - (DWORD)sizeof(SND_IMAGE_BASE_RELOCATION)) / (DWORD)sizeof(WORD);
    if (entry_index >= total_entries) {
        return SND_ERR(SND_STATUS_RELOCATION_ENTRY_OVERFLOW);
    }

    const WORD *entries = (const WORD *)SND_PTR_ADD(block, sizeof(SND_IMAGE_BASE_RELOCATION));
    WORD        entry   = entries[entry_index];
    WORD        type    = entry >> 12;
    WORD        offset  = entry & SND_PAGE_OFFSET_MASK;

    out_entry->type = type;
    if (type == SND_IMAGE_REL_BASED_ABSOLUTE) {
        return SND_OK;
    }

    if (SND_ADD_OVERFLOWS_DWORD(block->VirtualAddress, offset)) {
        return SND_ERR(SND_STATUS_RELOCATION_ENTRY_OVERFLOW);
    }

    DWORD patch_rva      = block->VirtualAddress + offset;
    out_entry->patch_rva = patch_rva;

    SIZE_T patch_size = 0;
    switch (type) {
    case SND_IMAGE_REL_BASED_HIGHLOW:
        patch_size = sizeof(DWORD);
        break;
    case SND_IMAGE_REL_BASED_DIR64:
        patch_size = sizeof(ULONGLONG);
        break;
    case SND_IMAGE_REL_BASED_HIGH:
    case SND_IMAGE_REL_BASED_LOW:
        patch_size = sizeof(WORD);
        break;
    default:
        return SND_ERR_CTX(SND_STATUS_PE_RELOCATION_TYPE_UNSUPPORTED, "Unsupported relocation type: %d", type);
    }

    PVOID patch_ptr = snd_pe_rva_to_ptr(parser, patch_rva, patch_size);
    if (!patch_ptr) {
        return SND_ERR(SND_STATUS_PE_RELOCATION_PATCH_OUT_OF_RANGE);
    }

    out_entry->patch_ptr = patch_ptr;
    return SND_OK;
}
