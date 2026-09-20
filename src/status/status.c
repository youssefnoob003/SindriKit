#include <sindri/common/debug.h>
#include <sindri/loaders/coff/status.h>
#include <sindri/loaders/pe/status.h>
#include <sindri/parsers/coff/status.h>
#include <sindri/parsers/env/status.h>
#include <sindri/parsers/pe/status.h>
#include <sindri/primitives/status.h>
#include <sindri/primitives/syscalls.h>
#include <sindri/status.h>

#if SND_DEBUG
#include <stdio.h> // For stderr
#endif

const char *snd_status_to_string(snd_status_t status) {
#if SND_DEBUG
    switch (SND_STATUS_FACILITY(status.code)) {
    case SND_FACILITY_GENERIC:
        return snd_generic_status_to_string(status.code);

    case SND_FACILITY_CLI:
        return snd_cli_status_to_string(status.code);

    case SND_FACILITY_FILE:
        return snd_file_status_to_string(status.code);

    case SND_FACILITY_PARSER_PE:
        return snd_pe_parser_status_to_string(status.code);

    case SND_FACILITY_PARSER_COFF:
        return snd_coff_parser_status_to_string(status.code);

    case SND_FACILITY_PARSER_ENV:
        return snd_env_parser_status_to_string(status.code);

    case SND_FACILITY_LOADER_PE:
        return snd_ldr_pe_status_to_string(status.code);

    case SND_FACILITY_LOADER_COFF:
        return snd_ldr_coff_status_to_string(status.code);

    case SND_FACILITY_SYSCALL:
        return snd_syscall_status_to_string(status.code);

    case SND_FACILITY_PRIMITIVES:
        return snd_prim_status_to_string(status.code);

    case SND_FACILITY_CONTEXT_MACHINES:
        return snd_context_machines_status_to_string(status.code);

    default:
        return "Unkown Facilty";
    }
#else
    (void)status;
    return "";
#endif
}

void snd_status_print(snd_status_t status) {
#if SND_DEBUG
    if (SND_SUCCEEDED(status)) {
        return;
    }

    const char *desc = snd_status_to_string(status);
    if (status.context[0] != '\0') {
        if (status.os_error != 0) {
            SND_FDEBUG_PRINT(stderr, "[ERR] 0x%08X: %s (%s, OS Error: 0x%08X) at %s:%d\n", status.code, desc,
                             status.context, status.os_error, status.file ? status.file : "unknown", status.line);
        } else {
            SND_FDEBUG_PRINT(stderr, "[ERR] 0x%08X: %s (%s) at %s:%d\n", status.code, desc, status.context,
                             status.file ? status.file : "unknown", status.line);
        }
    } else {
        if (status.os_error != 0) {
            SND_FDEBUG_PRINT(stderr, "[ERR] 0x%08X: %s (OS Error: 0x%08X) at %s:%d\n", status.code, desc,
                             status.os_error, status.file ? status.file : "unknown", status.line);
        } else {
            SND_FDEBUG_PRINT(stderr, "[ERR] 0x%08X: %s at %s:%d\n", status.code, desc,
                             status.file ? status.file : "unknown", status.line);
        }
    }
#else
    (void)status;
#endif
}
