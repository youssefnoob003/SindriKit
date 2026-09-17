#include <unified/cli.h>
#include <unified/common.h>
#include <unified/print.h>

int require_arg(int argc, int idx, const char *name) {
    if (idx + 1 >= argc) {
        log_err("Missing value for %s.", name);
        return -1;
    }
    return 0;
}

int parse_backend(int argc, char *argv[], int *idx, api_backend_t *out) {
    (void)argc;
    const char *a = argv[*idx];
    if (poc_strcmp(a, "--win") == 0) {
        *out = API_WIN;
        return 0;
    }
    if (poc_strcmp(a, "--nt") == 0) {
        *out = API_NT;
        return 0;
    }
    if (poc_strcmp(a, "--sys") == 0) {
        *out = API_SYS;
        return 0;
    }
    return -1;
}

int parse_syscall_style(int argc, char *argv[], int *idx, syscall_style_t *out) {
    (void)argc;
    const char *a = argv[*idx];
    if (poc_strcmp(a, "--invoke-direct") == 0) {
        out->invoke = INVOKE_DIRECT;
        return 0;
    }
    if (poc_strcmp(a, "--invoke-indirect") == 0) {
        out->invoke = INVOKE_INDIRECT;
        return 0;
    }
    if (poc_strcmp(a, "--invoke-spoofed") == 0) {
        out->invoke = INVOKE_SPOOFED;
        return 0;
    }
    if (poc_strcmp(a, "--resolve-scan") == 0) {
        out->resolve_scan = 1;
        return 0;
    }
    if (poc_strcmp(a, "--resolve-sort") == 0) {
        out->resolve_sort = 1;
        return 0;
    }
    if (poc_strcmp(a, "--sys-cache") == 0) {
        out->cache = 1;
        return 0;
    }
    return -1;
}

snd_status_t parse_call_arg(int argc, char *argv[], int *idx, UINT_PTR *args, DWORD *count) {
    if (require_arg(argc, *idx, "-a")) {
        return SND_ERR(SND_STATUS_MISSING_COMMAND_LINE_ARGS);
    }

    if (*count >= SND_MAX_CALL_ARGS) {
        log_err("Too many -a arguments (max %d).", SND_MAX_CALL_ARGS);
        return SND_ERR(SND_STATUS_INVALID_COMMAND_LINE_ARG);
    }

    const char        *raw    = argv[++(*idx)];
    char              *endptr = NULL;
    unsigned long long parsed = poc_strtoull(raw, &endptr, 0);

    if (endptr != raw && *endptr == '\0') {
        args[*count] = (UINT_PTR)parsed;
        log_info("arg[%lu] = 0x%llX (numeric)", (unsigned long)*count, parsed);
    } else {
        args[*count] = (UINT_PTR)raw;
        log_info("arg[%lu] = \"%s\" (string ptr: 0x%p)", (unsigned long)*count, raw, (void *)args[*count]);
    }

    (*count)++;
    return SND_OK;
}

snd_status_t parse_bof_arg(int argc, char *argv[], int *idx, char **args, int *arg_len) {
    if (require_arg(argc, *idx, "-a")) {
        return SND_ERR(SND_STATUS_MISSING_COMMAND_LINE_ARGS);
    }

    *args    = argv[++(*idx)];
    *arg_len = (int)poc_strlen(*args) + 1;
    return SND_OK;
}

const char *unified_backend_name(api_backend_t backend) {
    switch (backend) {
    case API_WIN:
        return "Win32";
    case API_NT:
        return "native NT";
    case API_SYS:
        return "syscall";
    default:
        return "unknown";
    }
}
