#ifndef SND_STATUS_CORE_H
#define SND_STATUS_CORE_H

#if SND_DEBUG
#include <stdarg.h>
#include <stdio.h>
#endif

#include <sindri/common/macros.h>
#include <sindri/status/facility.h>

SND_BEGIN_EXTERN_C

#define SND_MAX_CTX_LEN  128
#define SND_FAILED(x)    (x.code != SND_SUCCESS)
#define SND_SUCCEEDED(x) (x.code == SND_SUCCESS)

typedef enum _SND_CORE_STATUS_CODE {
    SND_SUCCESS       = 0,
    SND_ERROR_GENERIC = SND_MAKE_STATUS(SND_FACILITY_GENERIC, 1),
    SND_STATUS_NULL_POINTER,
    SND_STATUS_INVALID_PARAMETERS_COMBINATION,
    SND_STATUS_UNSUPPORTED,
    SND_STATUS_ARCH_MISMATCH,
    SND_STATUS_TOO_MANY_ARGUMENTS,

    // Command-line argument errors
    SND_STATUS_MISSING_COMMAND_LINE_ARGS = SND_MAKE_STATUS(SND_FACILITY_CLI, 1),
    SND_STATUS_INVALID_COMMAND_LINE_ARG,

    // File / input errors
    SND_STATUS_FILE_INVALID_PATH = SND_MAKE_STATUS(SND_FACILITY_FILE, 1),
    SND_STATUS_FILE_CREATE_FAILED,
    SND_STATUS_FILE_SIZE_QUERY_FAILED,
    SND_STATUS_FILE_TOO_LARGE,
    SND_STATUS_FILE_TOO_SMALL,
    SND_STATUS_FILE_READ_FAILED,
    SND_STATUS_FILE_ALLOC_FAILED,

    // Context machine errors
    SND_STATUS_INVALID_STAGE = SND_MAKE_STATUS(SND_FACILITY_CONTEXT_MACHINES, 1),
    SND_STATUS_CORRUPTED_STAGE,
} snd_core_status_code_t;

typedef int snd_status_code_t;

typedef struct _SND_STATUS {
    snd_status_code_t code;
    int               os_error;
#if SND_DEBUG
    const char *file;
    int         line;
    char        context[SND_MAX_CTX_LEN];
#endif
} snd_status_t;

#if SND_DEBUG

static inline snd_status_t _snd_make_err(snd_status_code_t code, int os_error, const char *file, int line,
                                         const char *fmt, ...) {
    snd_status_t status = {code, os_error, file, line};

    if (fmt != NULL) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(status.context, SND_MAX_CTX_LEN, fmt, args);
        va_end(args);
    } else {
        status.context[0] = '\0';
    }

    return status;
}

static inline snd_status_t _snd_make_success() {
    snd_status_t status = {SND_SUCCESS, 0, NULL, 0, {0}};

    return status;
}

#define SND_OK                               _snd_make_success()
#define SND_ERR(c)                           _snd_make_err(c, 0, __FILE__, __LINE__, NULL)
#define SND_ERR_CTX(c, ...)                  _snd_make_err(c, 0, __FILE__, __LINE__, __VA_ARGS__)
#define SND_ERR_W32(code)                    _snd_make_err(code, GetLastError(), __FILE__, __LINE__, NULL)
#define SND_ERR_W32_CTX(code, ...)           _snd_make_err(code, GetLastError(), __FILE__, __LINE__, __VA_ARGS__)
#define SND_ERR_NT(code, nt_status)          _snd_make_err(code, nt_status, __FILE__, __LINE__, NULL)
#define SND_ERR_NT_CTX(code, nt_status, ...) _snd_make_err(code, nt_status, __FILE__, __LINE__, __VA_ARGS__)
#else

static inline snd_status_t _snd_make_success() {
    snd_status_t status = {SND_SUCCESS, 0};

    return status;
}

static inline snd_status_t _snd_make_err(snd_status_code_t code, int os_error) {
    snd_status_t status = {code, os_error};

    return status;
}

#define SND_OK                               _snd_make_success()
#define SND_ERR(code)                        _snd_make_err(code, 0)
#define SND_ERR_CTX(code, ...)               _snd_make_err(code, 0)
#define SND_ERR_W32(code)                    _snd_make_err(code, GetLastError())
#define SND_ERR_W32_CTX(code, ...)           _snd_make_err(code, GetLastError())
#define SND_ERR_NT(code, nt_status)          _snd_make_err(code, nt_status)
#define SND_ERR_NT_CTX(code, nt_status, ...) _snd_make_err(code, nt_status)

#endif

#define SND_CHECK_NULL_1(p)                                                                                            \
    do {                                                                                                               \
        if (!(p)) {                                                                                                    \
            return SND_ERR_CTX(SND_STATUS_NULL_POINTER, "Parameter '%s' is NULL", #p);                                 \
        }                                                                                                              \
    } while (0)

#define SND_CHECK_NULL_2(p1, p2)                                                                                       \
    SND_CHECK_NULL_1(p1);                                                                                              \
    SND_CHECK_NULL_1(p2)
#define SND_CHECK_NULL_3(p1, p2, p3)                                                                                   \
    SND_CHECK_NULL_2(p1, p2);                                                                                          \
    SND_CHECK_NULL_1(p3)
#define SND_CHECK_NULL_4(p1, p2, p3, p4)                                                                               \
    SND_CHECK_NULL_3(p1, p2, p3);                                                                                      \
    SND_CHECK_NULL_1(p4)
#define SND_CHECK_NULL_5(p1, p2, p3, p4, p5)                                                                           \
    SND_CHECK_NULL_4(p1, p2, p3, p4);                                                                                  \
    SND_CHECK_NULL_1(p5)

#define SND_GET_CHECK_NULL_MACRO(_1, _2, _3, _4, _5, NAME, ...) NAME

#define SND_CHECK_NULL(...)                                                                                            \
    SND_GET_CHECK_NULL_MACRO(__VA_ARGS__, SND_CHECK_NULL_5, SND_CHECK_NULL_4, SND_CHECK_NULL_3, SND_CHECK_NULL_2,      \
                             SND_CHECK_NULL_1)(__VA_ARGS__)

#define SND_TRY(expr)                                                                                                  \
    do {                                                                                                               \
        snd_status_t _status = (expr);                                                                                 \
        if (SND_FAILED(_status)) {                                                                                     \
            return _status;                                                                                            \
        }                                                                                                              \
    } while (0)

/**
 * @brief Converts a generic status code to a human-readable string.
 * @param code Status code to convert.
 * @retval Pointer to a null-terminated description string.
 */
const char *snd_generic_status_to_string(int code);

/**
 * @brief Converts a CLI status code to a human-readable string.
 * @param code Status code to convert.
 * @retval Pointer to a null-terminated description string.
 */
const char *snd_cli_status_to_string(int code);

/**
 * @brief Converts a file status code to a human-readable string.
 * @param code Status code to convert.
 * @retval Pointer to a null-terminated description string.
 */
const char *snd_file_status_to_string(int code);

/**
 * @brief Converts a context machines status code to a human-readable string.
 * @param code Status code to convert.
 * @retval Pointer to a null-terminated description string.
 */
const char *snd_context_machines_status_to_string(int code);

/**
 * @brief Converts a status structure into a human-readable string.
 * @param status The status object to evaluate.
 * @retval Pointer to a null-terminated string describing the status code.
 */
const char *snd_status_to_string(snd_status_t status);

/**
 * @brief Prints a status structure, including context if available, to stderr.
 * @param status The status object to print.
 */
void snd_status_print(snd_status_t status);

SND_END_EXTERN_C

#endif // SND_STATUS_CORE_H
