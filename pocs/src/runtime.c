#include <unified/runtime.h>

#if defined(SND_CRTLESS)

int poc_strcmp(const char *left, const char *right) {
    return snd_strncmp(left, right, (size_t)-1);
}

size_t poc_strlen(const char *value) {
    return snd_strnlen(value, (size_t)-1);
}

static unsigned long long poc_parse_unsigned(const char *value, char **end, int base) {
    const char        *cursor = value;
    unsigned long long result = 0;
    int                digits = 0;

    if (base == 0) {
        base = 10;
        if (cursor[0] == '0' && (cursor[1] == 'x' || cursor[1] == 'X')) {
            base = 16;
            cursor += 2;
        }
    } else if (base == 16 && cursor[0] == '0' && (cursor[1] == 'x' || cursor[1] == 'X')) {
        cursor += 2;
    }

    while (*cursor) {
        unsigned int digit;
        if (*cursor >= '0' && *cursor <= '9')
            digit = (unsigned int)(*cursor - '0');
        else if (*cursor >= 'a' && *cursor <= 'f')
            digit = (unsigned int)(*cursor - 'a') + 10;
        else if (*cursor >= 'A' && *cursor <= 'F')
            digit = (unsigned int)(*cursor - 'A') + 10;
        else
            break;
        if (digit >= (unsigned int)base)
            break;
        result = result * (unsigned int)base + digit;
        cursor++;
        digits++;
    }

    if (end)
        *end = (char *)(digits ? cursor : value);
    return result;
}

unsigned long poc_strtoul(const char *value, char **end, int base) {
    return (unsigned long)poc_parse_unsigned(value, end, base);
}

unsigned long long poc_strtoull(const char *value, char **end, int base) {
    return poc_parse_unsigned(value, end, base);
}

#else

#include <stdlib.h>
#include <string.h>

int poc_strcmp(const char *left, const char *right) {
    return strcmp(left, right);
}

size_t poc_strlen(const char *value) {
    return strlen(value);
}

unsigned long poc_strtoul(const char *value, char **end, int base) {
    return strtoul(value, end, base);
}

unsigned long long poc_strtoull(const char *value, char **end, int base) {
    return strtoull(value, end, base);
}

#endif
