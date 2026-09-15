#include <sindri/parsers/coff/status.h>

const char *snd_coff_parser_status_to_string(int code) {
#if SND_DEBUG
    switch (code) {
    // Headers
    case SND_STATUS_HEADER_FILE_TRUNCATED:
        return "COFF buffer truncated before file header";
    case SND_STATUS_HEADER_MACHINE_UNSUPPORTED:
        return "Unsupported COFF target machine architecture";

    // Sections
    case SND_STATUS_SECTION_TABLE_TRUNCATED:
        return "COFF section header table extends beyond safe buffer bounds";
    case SND_STATUS_SECTION_OFFSET_OVERFLOW:
        return "COFF section offset or size integer overflow";
    case SND_STATUS_SECTION_HEADER_MISSING:
        return "Requested COFF section header not found";

    // Symbols
    case SND_STATUS_SYMBOL_TABLE_TRUNCATED:
        return "COFF symbol table extends beyond safe buffer bounds";
    case SND_STATUS_SYMBOL_TABLE_OVERFLOW:
        return "COFF symbol table offset or count integer overflow";
    case SND_STATUS_SYMBOL_ENTRY_MISSING:
        return "Requested COFF symbol entry not found";
    case SND_STATUS_SYMBOL_RESOLUTION_FAILED:
        return "Failed to resolve COFF external or internal symbol";
    case SND_STATUS_SYMBOL_NAKED_REJECTED:
        return "Naked or unresolvable COFF symbol rejected";
    case SND_STATUS_SYMBOL_DECODE_OVERFLOW:
        return "COFF symbol decode value integer overflow";

    // Strings
    case SND_STATUS_STRING_TABLE_OVERFLOW:
        return "COFF string table offset integer overflow";

    // Relocations
    case SND_STATUS_RELOCATION_TABLE_INVALID:
        return "COFF relocation table contains invalid header or count";
    case SND_STATUS_RELOCATION_TABLE_MISSING:
        return "COFF section relocation table is missing";
    case SND_STATUS_RELOCATION_TABLE_TRUNCATED:
        return "COFF relocation table extends beyond safe buffer bounds";
    case SND_STATUS_RELOCATION_TABLE_OVERFLOW:
        return "COFF relocation table offset or count integer overflow";
    case SND_STATUS_COFF_RELOCATION_TYPE_UNSUPPORTED:
        return "Unsupported COFF relocation entry type";
    case SND_STATUS_COFF_RELOCATION_PATCH_OUT_OF_RANGE:
        return "COFF relocation target offset falls outside section bounds";
    default:
        return "Unknown error code";
    }
#else
    (void)code;
    return "";
#endif
}
