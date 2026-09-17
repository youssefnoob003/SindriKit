#ifndef UNIFIED_CLI_H
#define UNIFIED_CLI_H

#include <unified/common.h>

int          require_arg(int argc, int idx, const char *name);
int          parse_backend(int argc, char *argv[], int *idx, api_backend_t *out);
int          parse_syscall_style(int argc, char *argv[], int *idx, syscall_style_t *out);
snd_status_t parse_call_arg(int argc, char *argv[], int *idx, UINT_PTR *args, DWORD *count);
snd_status_t parse_bof_arg(int argc, char *argv[], int *idx, char **args, int *arg_len);
const char  *unified_backend_name(api_backend_t backend);

#endif
