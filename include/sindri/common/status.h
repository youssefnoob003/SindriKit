#ifndef SND_COMMON_STATUS_H
#define SND_COMMON_STATUS_H

#include <windows.h>

#if SND_DEBUG
#include <stdarg.h>
#include <stdio.h>
#endif

#include <sindri/common/helpers.h>

SND_BEGIN_EXTERN_C

// Maximum length for error context strings (including null terminator)
#define SND_MAX_CTX_LEN 128

typedef enum _SND_STATUS_CODE {
    SND_SUCCESS                  = 0,
    SND_ERROR_GENERIC            = -1,
    SND_STATUS_INVALID_PARAMETER = -2,
    SND_STATUS_INTEGER_OVERFLOW  = -3,
    SND_STATUS_UNSUPPORTED       = -4,
    SND_STATUS_NOT_INITIALIZED   = -5,

    // Command-line argument errors
    SND_STATUS_MISSING_COMMAND_LINE_ARGS = 0x100,
    SND_STATUS_INVALID_COMMAND_LINE_ARG,

    // File / input errors
    SND_STATUS_INVALID_PATH = 0x200,
    SND_STATUS_FILE_CREATE_FAILED,
    SND_STATUS_FILE_SIZE_QUERY_FAILED,
    SND_STATUS_FILE_TOO_LARGE,
    SND_STATUS_FILE_TOO_SMALL,
    SND_STATUS_ALLOC_FAILED,
    SND_STATUS_FILE_READ_FAILED,

    // PE parsing / validation errors
    SND_STATUS_DOS_HEADER_TRUNCATED = 0x300,
    SND_STATUS_INVALID_DOS_SIGNATURE,
    SND_STATUS_NT_HEADERS_TRUNCATED,
    SND_STATUS_INVALID_NT_SIGNATURE,
    SND_STATUS_INVALID_NT_HEADER_OFFSET,
    SND_STATUS_UNSUPPORTED_OPTIONAL_HEADER_MAGIC,
    SND_STATUS_INVALID_SECTION_COUNT,
    SND_STATUS_SECTION_TABLE_OVERFLOW,
    SND_STATUS_INVALID_SECTION_TABLE,
    SND_STATUS_ARCH_MISMATCH,
    SND_STATUS_ENTRY_POINT_NOT_FOUND,
    SND_STATUS_INVALID_IMPORT_TABLE_SIZE,
    SND_STATUS_INVALID_FILE_OFFSET,
    SND_STATUS_DIRECTORY_NOT_FOUND,

    // Reflective loading errors
    SND_STATUS_SECTION_COPY_FAILED = 0x400,
    SND_STATUS_RELOCATION_DIRECTORY_INVALID,
    SND_STATUS_RELOCATION_PATCH_OUT_OF_RANGE,
    SND_STATUS_RELOCATION_TYPE_UNSUPPORTED,
    SND_STATUS_IMPORT_DIRECTORY_INVALID,
    SND_STATUS_IMPORT_DLL_LOAD_FAILED,
    SND_STATUS_IMPORT_THUNK_INVALID,
    SND_STATUS_IMPORT_SYMBOL_RESOLVE_FAILED,
    SND_STATUS_PROTECTION_UPDATE_FAILED,
    SND_STATUS_DLL_INITIALIZATION_FAILED,
    SND_STATUS_EXPORT_DIRECTORY_INVALID,
    SND_STATUS_EXPORT_NOT_FOUND,
    SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED,
    SND_STATUS_MISSING_DLL_EXPORT_NAME,
    SND_STATUS_INVALID_STAGE_SEQUENCE,
    SND_STATUS_CORRUPTED_STATE,
    SND_FAILED_TO_EXECUTE,

    // Syscall resolution errors
    SND_STATUS_SSN_NOT_FOUND,
    SND_STATUS_SSN_BUFFER_TOO_SMALL,

    // PEB
    SND_STATUS_PEB_MODULE_NOT_FOUND,
} snd_status_code_t;

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

#define SND_OK                     _snd_make_success()
#define SND_ERR(c)                 _snd_make_err(c, 0, __FILE__, __LINE__, NULL)
#define SND_ERR_CTX(c, ...)        _snd_make_err(c, 0, __FILE__, __LINE__, __VA_ARGS__)
#define SND_ERR_W32(code)          _snd_make_err(code, GetLastError(), __FILE__, __LINE__, NULL)
#define SND_ERR_W32_CTX(code, ...) _snd_make_err(code, GetLastError(), __FILE__, __LINE__, __VA_ARGS__)

#else

static inline snd_status_t _snd_make_success() {
    snd_status_t status = {SND_SUCCESS, 0};

    return status;
}

static inline snd_status_t _snd_make_err(snd_status_code_t code, int os_error) {
    snd_status_t status = {code, os_error};

    return status;
}

#define SND_OK                     _snd_make_success()
#define SND_ERR(code)              _snd_make_err(code, 0)
#define SND_ERR_CTX(code, ...)     _snd_make_err(code, 0)
#define SND_ERR_W32(code)          _snd_make_err(code, GetLastError())
#define SND_ERR_W32_CTX(code, ...) _snd_make_err(code, GetLastError())

#endif

/**
 * @brief Converts a status structure into a human-readable string.
 * @param status The status object to evaluate.
 * @return Null-terminated string describing the status code.
 */
const char *snd_status_to_string(snd_status_t status);

/**
 * @brief Prints a status structure, including context if available, to stderr.
 * @param status The status object to print.
 */
void snd_status_print(snd_status_t status);

SND_END_EXTERN_C

#endif // SND_COMMON_STATUS_H
