#include <sindri/parsers/pe/status.h>

const char *snd_pe_parser_status_to_string(int code) {
#if SND_DEBUG
    switch ((snd_pe_parser_status_code_t)code) {
    // Headers
    case SND_STATUS_HEADER_DOS_TRUNCATED:
        return "PE buffer truncated before DOS header";
    case SND_STATUS_HEADER_DOS_SIGNATURE_INVALID:
        return "Invalid PE DOS signature";
    case SND_STATUS_HEADER_NT_TRUNCATED:
        return "PE buffer truncated before NT headers";
    case SND_STATUS_HEADER_NT_SIGNATURE_INVALID:
        return "Invalid PE NT signature";
    case SND_STATUS_HEADER_OPTIONAL_SIGNATURE_INVALID:
        return "Invalid PE optional header signature";
    case SND_STATUS_HEADER_OFFSET_INVALID:
        return "Invalid PE NT header offset";

    // Data Directories
    case SND_STATUS_DIRECTORY_ENTRY_MISSING:
        return "Requested PE data directory entry is missing";
    case SND_STATUS_DIRECTORY_ENTRY_INVALID:
        return "PE data directory entry extends past image boundary";

    // Import Directory
    case SND_STATUS_IMPORT_DESCRIPTOR_INVALID:
        return "PE import directory descriptor table is invalid or truncated";
    case SND_STATUS_IMPORT_DESCRIPTOR_OVERFLOW:
        return "PE import directory descriptor RVA or stride integer overflow";
    case SND_STATUS_IMPORT_NAME_INVALID:
        return "PE import function name is invalid or unmapped";
    case SND_STATUS_IMPORT_THUNK_INVALID:
        return "PE import thunk table entry is invalid or truncated";
    case SND_STATUS_IMPORT_THUNK_OUT_OF_BOUNDS:
        return "PE import thunk table entry points outside mapped image bounds";
    case SND_STATUS_IMPORT_THUNK_OVERFLOW:
        return "PE import thunk stride or RVA integer overflow";

    // Export Directory
    case SND_STATUS_EXPORT_DIRECTORY_NULL:
        return "PE export directory is missing or NULL";
    case SND_STATUS_EXPORT_DIRECTORY_INVALID:
        return "PE export directory table is invalid or truncated";
    case SND_STATUS_EXPORT_DIRECTORY_OVERFLOW:
        return "PE export directory table RVA integer overflow";
    case SND_STATUS_EXPORT_TABLE_EMPTY:
        return "PE export directory contains 0 exported functions";
    case SND_STATUS_EXPORT_SYMBOL_MISSING:
        return "Requested export function name or ordinal not found";
    case SND_STATUS_EXPORT_ORDINAL_INVALID:
        return "Export ordinal value or function address calculation is invalid";
    case SND_STATUS_EXPORT_ORDINAL_OVERFLOW:
        return "PE export ordinal index or address calculation integer overflow";
    case SND_STATUS_EXPORT_ORDINAL_OUT_OF_RANGE:
        return "Requested export ordinal is out of directory bounds";
    case SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED:
        return "Forwarded export encountered but no module resolver was provided";
    case SND_STATUS_EXPORT_FORWARDER_EXCEEDED:
        return "Maximum export forwarder recursion depth exceeded";
    case SND_STATUS_EXPORT_FORWARDER_INVALID:
        return "Export forwarder target string is malformed";

    // Relocation Directory
    case SND_STATUS_RELOCATION_DIRECTORY_TRUNCATED:
        return "PE base relocation directory is truncated";
    case SND_STATUS_RELOCATION_DIRECTORY_OUT_OF_BOUNDS:
        return "PE base relocation directory extends beyond image boundary";
    case SND_STATUS_RELOCATION_DIRECTORY_OVERFLOW:
        return "PE base relocation block RVA or size integer overflow";
    case SND_STATUS_RELOCATION_BLOCK_INVALID:
        return "PE base relocation block header or size is invalid";
    case SND_STATUS_RELOCATION_ENTRY_OVERFLOW:
        return "PE base relocation block entry index integer overflow";
    case SND_STATUS_PE_RELOCATION_PATCH_OUT_OF_RANGE:
        return "Relocation patch virtual address is outside mapped image boundary";
    case SND_STATUS_PE_RELOCATION_TYPE_UNSUPPORTED:
        return "Unsupported PE base relocation block entry type";

    default:
        return "Unknown PE parser status code";
    }
#else
    (void)code;
    return "";
#endif
}
