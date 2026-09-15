#ifndef SND_PARSERS_ENV_STATUS_H
#define SND_PARSERS_ENV_STATUS_H

#include <sindri/common/macros.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

typedef enum _SND_ENV_PARSER_STATUS_CODE {
    // PEB
    SND_STATUS_PEB_GET_FAILED = SND_MAKE_STATUS(SND_FACILITY_PARSER_ENV, 1),
    SND_STATUS_PEB_LDR_NOT_INITIALIZED,
    SND_STATUS_PROCESS_PARAMS_NOT_FOUND,
    SND_STATUS_MODULE_LIST_CORRUPTED,
    SND_STATUS_MODULE_NOT_FOUND,
    SND_STATUS_NTDLL_CLEAN_NOT_INITIALIZED,
} snd_env_parser_status_code_t;

/**
 * @brief Converts an environment parser status code into a human-readable description string.
 * @param code Error code value.
 * @retval Pointer to a string description, or the default error message.
 */
const char *snd_env_parser_status_to_string(int code);

SND_END_EXTERN_C

#endif // SND_PARSERS_ENV_STATUS_H
