#include <sindri/internal/nt/api.h>
#include <sindri/parsers/env/ntdll.h>
#include <sindri/parsers/env/peb.h>
#include <sindri_hashes.h>
#include <unified/main.h>

#define POC_MAX_ARGS 32
#define POC_ARG_SIZE 512

static char  poc_args[POC_MAX_ARGS][POC_ARG_SIZE];
static char *poc_argv[POC_MAX_ARGS];

static int build_argv(const wchar_t *command_line, int *argc_out, char ***argv_out) {
    int            argc   = 0;
    const wchar_t *cursor = command_line;

    while (*cursor && argc < POC_MAX_ARGS - 1) {
        while (*cursor == L' ' || *cursor == L'\t')
            cursor++;
        if (!*cursor)
            break;

        const wchar_t *start  = cursor;
        int            quoted = 0;
        size_t         length = 0;
        while (*cursor) {
            if (*cursor == L'"') {
                quoted = !quoted;
            } else if (!quoted && (*cursor == L' ' || *cursor == L'\t')) {
                break;
            }
            cursor++;
            length++;
        }

        size_t written = 0;
        for (size_t i = 0; i < length && written + 1 < POC_ARG_SIZE; i++) {
            if (start[i] != L'"')
                poc_args[argc][written++] = (char)(start[i] <= 0x7f ? start[i] : '?');
        }
        poc_args[argc][written] = '\0';
        poc_argv[argc]          = poc_args[argc];
        argc++;
    }

    poc_argv[argc] = NULL;
    *argc_out      = argc;
    *argv_out      = poc_argv;
    return argc > 0 ? 0 : -1;
}

void snd_crtless_poc_entry(void) {
    SND_UNICODE_STRING *command_line = NULL;
    snd_status_t        status       = snd_env_get_command_line(NULL, &command_line);
    int                 argc         = 0;
    char              **argv         = NULL;
    if (SND_SUCCEEDED(status) && build_argv(command_line ? command_line->Buffer : NULL, &argc, &argv) == 0)
        status.code = unified_main(argc, argv);
    else if (SND_SUCCEEDED(status))
        status.code = SND_STATUS_MISSING_COMMAND_LINE_ARGS;

    FARPROC exit_address = NULL;
    if (SND_SUCCEEDED(snd_ntdll_get_active_export(SND_HASH_RTLEXITUSERPROCESS, &exit_address)))
        ((SND_RtlExitUserProcess_t)exit_address)((NTSTATUS)status.code);
    for (;;) {
    }
}
