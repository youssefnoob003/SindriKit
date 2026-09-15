#include <sindri/common/debug.h>
#include <sindri/status.h>

const char *snd_generic_status_to_string(int code) {
#if SND_DEBUG
    switch (code) {
    case SND_SUCCESS:
        return "Operation completed successfully";
    case SND_ERROR_GENERIC:
        return "An unspecified error occurred";
    case SND_STATUS_NULL_POINTER:
        return "Null pointer provided";
    case SND_STATUS_INVALID_PARAMETERS_COMBINATION:
        return "Invalid parameters combination provided";
    case SND_STATUS_UNSUPPORTED:
        return "Unsupported operation";
    case SND_STATUS_ARCH_MISMATCH:
        return "Architecture incompatible with target payload";
    case SND_STATUS_TOO_MANY_ARGUMENTS:
        return "Too many arguments provided";
    default:
        return "Unkown error code";
    }
#else
    (void)code;
    return "";
#endif
}

const char *snd_context_machines_status_to_string(int code) {
#if SND_DEBUG
    switch (code) {
    case SND_STATUS_INVALID_STAGE:
        return "Operation invoked at an invalid pipeline stage";
    case SND_STATUS_CORRUPTED_STAGE:
        return "Operation context machine state is corrupted";
    default:
        return "Unknown context machine error code";
    }
#else
    (void)code;
    return "";
#endif
}
