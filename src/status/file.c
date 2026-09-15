#include <sindri/status.h>

const char *snd_file_status_to_string(int code) {
#if SND_DEBUG
    switch (code) {
    case SND_STATUS_FILE_INVALID_PATH:
        return "Payload path is invalid";
    case SND_STATUS_FILE_CREATE_FAILED:
        return "Failed to open specified file";
    case SND_STATUS_FILE_SIZE_QUERY_FAILED:
        return "Failed to query file size";
    case SND_STATUS_FILE_TOO_LARGE:
        return "File too large";
    case SND_STATUS_FILE_TOO_SMALL:
        return "File too small";
    case SND_STATUS_FILE_READ_FAILED:
        return "Failed to read file";
    case SND_STATUS_FILE_ALLOC_FAILED:
        return "Allocation failed for file loading";
    default:
        return "Unkown error code";
    }
#else
    (void)code;
    return "";
#endif
}
