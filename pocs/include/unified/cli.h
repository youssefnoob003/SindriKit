#ifndef UNIFIED_CLI_H
#define UNIFIED_CLI_H

#include <unified/common.h>

int require_arg(int argc, char *argv[], int idx, const char *name);
int parse_backend(int argc, char *argv[], int *idx, api_backend_t *out);
int parse_syscall_style(int argc, char *argv[], int *idx, syscall_style_t *out);

#endif
