#ifndef UNIFIED_RUNTIME_H
#define UNIFIED_RUNTIME_H

#include <sindri/common/string.h>

int                poc_strcmp(const char *left, const char *right);
size_t             poc_strlen(const char *value);
unsigned long      poc_strtoul(const char *value, char **end, int base);
unsigned long long poc_strtoull(const char *value, char **end, int base);

#endif
