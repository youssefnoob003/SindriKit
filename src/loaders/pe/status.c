#include <sindri/loaders/pe/status.h>

const char *snd_ldr_pe_status_to_string(int code) {
#if SND_DEBUG
    switch ((snd_pe_loader_status_code_t)code) {
    case SND_STATUS_HEADERS_SIZE_INVALID:
        return "PE headers size is invalid or extends past buffer bounds";
    case SND_STATUS_SECTION_TABLE_MISSING:
        return "PE section table is missing";
    case SND_STATUS_SECTION_SIZE_INVALID:
        return "PE section size or virtual layout parameter is invalid";
    case SND_STATUS_RELOCATION_DIRECTORY_MISSING:
        return "PE relocation directory is missing";
    case SND_STATUS_RELOCATION_TYPE_INVALID:
        return "Invalid or unsupported PE relocation type";
    case SND_STATUS_RELOCATION_DIRECTORY_STRIPPED:
        return "PE base relocation directory is missing or stripped";
    case SND_STATUS_IMAGE_SIZE_NULL:
        return "Calculated PE image size is zero";
    case SND_STATUS_DLL_INITIALIZATION_FAILED:
        return "DLL entry point returned failure";
    case SND_STATUS_LOCAL_EXECUTION_BLOCKED:
        return "Local PE execution is blocked by configuration";
    case SND_STATUS_IMAGE_ENTRY_POINT_MISSING:
        return "PE entry point address is missing";

    default:
        return "Unknown PE loader status code";
    }
#else
    (void)code;
    return "";
#endif
}
