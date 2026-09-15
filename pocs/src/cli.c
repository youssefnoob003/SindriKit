#include <unified/cli.h>
#include <unified/common.h>
#include <unified/print.h>

int require_arg(int argc, char *argv[], int idx, const char *name) {
    (void)argc;
    (void)argv;
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
#if defined(SND_CRTLESS)
        return -1;
#else
        *out = API_WIN;
        return 0;
#endif
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
    return -1;
}
