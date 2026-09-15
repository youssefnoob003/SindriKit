#include <sindri/status.h>

const char *snd_cli_status_to_string(int code) {
#if SND_DEBUG
    switch (code) {
    case SND_STATUS_MISSING_COMMAND_LINE_ARGS:
        return "Required command-line arguments are missing";
    case SND_STATUS_INVALID_COMMAND_LINE_ARG:
        return "Unknown or invalid command-line argument";
    default:
        return "Unkown error code";
    }
#else
    (void)code;
    return "";
#endif
}
