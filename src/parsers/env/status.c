#include <sindri/parsers/env/status.h>

const char *snd_env_parser_status_to_string(int code) {
#if SND_DEBUG
    switch ((snd_env_parser_status_code_t)code) {
    // PEB
    case SND_STATUS_PEB_GET_FAILED:
        return "Failed to query or locate PEB address";
    case SND_STATUS_PEB_LDR_NOT_INITIALIZED:
        return "PEB loader data structure is not initialized";
    case SND_STATUS_PROCESS_PARAMS_NOT_FOUND:
        return "Process parameters structure not found in PEB";
    case SND_STATUS_MODULE_LIST_CORRUPTED:
        return "PEB module memory list is corrupted or unreadable";
    case SND_STATUS_MODULE_NOT_FOUND:
        return "Target module not found in PEB loader module list";
    case SND_STATUS_NTDLL_CLEAN_NOT_INITIALIZED:
        return "Clean in-memory copy of ntdll is not initialized";

    default:
        return "Unknown environment parser status code";
    }
#else
    (void)code;
    return "";
#endif
}
