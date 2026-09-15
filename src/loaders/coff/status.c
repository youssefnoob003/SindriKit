#include <sindri/loaders/coff/status.h>

const char *snd_ldr_coff_status_to_string(int code) {
#if SND_DEBUG
    switch ((snd_coff_loader_status_code_t)code) {
    case SND_STATUS_COFF_LOADER_SYMBOL_MISSING:
        return "COFF symbol entry missing during loading";
    case SND_STATUS_SYMBOL_ENTRY_OUT_OF_RANGE:
        return "COFF symbol table entry index out of bounds";
    case SND_STATUS_SYMBOL_ADDRESS_NULL:
        return "Resolved COFF symbol address is NULL";
    case SND_STATUS_COFF_LOADER_RELOC_OUT_OF_RANGE:
        return "COFF relocation patch address out of section range";
    case SND_STATUS_RELOCATION_UNKNOWN_TYPE:
        return "Unknown or unsupported COFF relocation type";
    case SND_STATUS_COFF_LOADER_MAP_SIZE_OVERFLOW:
        return "COFF sections total mapping size exceeds maximum bounds";
    case SND_STATUS_COFF_LOADER_BSS_SIZE_OVERFLOW:
        return "COFF uninitialized data (BSS) size arithmetic overflow";
    case SND_STATUS_COFF_LOADER_TRAMPOLINE_OVERFLOW:
        return "COFF trampoline allocation failed due to insufficient space";
    case SND_STATUS_COFF_LOADER_VIRTUAL_SIZE_OVERFLOW:
        return "COFF virtual allocation size calculation overflow";

    default:
        return "Unknown COFF loader status code";
    }
#else
    (void)code;
    return "";
#endif
}
