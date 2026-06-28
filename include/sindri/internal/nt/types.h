#ifndef SND_INTERN_NT_TYPES_H
#define SND_INTERN_NT_TYPES_H

#include <sindri/common/macros.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Macro to check if an NTSTATUS indicates success.
 */
#define SND_NT_SUCCESS(s) (((NTSTATUS)(s)) >= 0)

/**
 * @brief Object attribute flag indicating case-insensitive object names.
 */
#define SND_OBJ_CASE_INSENSITIVE 0x00000040L

/**
 * @brief Page size used in memory allocations.
 */
#define SND_PAGE_SIZE ((SIZE_T)0x1000)

/**
 * @brief Initializes an SND_OBJECT_ATTRIBUTES structure.
 */
#define SND_InitializeObjectAttributes(p, n, a, r, s)                                                                  \
    {                                                                                                                  \
        (p)->Length                   = sizeof(SND_OBJECT_ATTRIBUTES);                                                 \
        (p)->RootDirectory            = r;                                                                             \
        (p)->Attributes               = a;                                                                             \
        (p)->ObjectName               = n;                                                                             \
        (p)->SecurityDescriptor       = s;                                                                             \
        (p)->SecurityQualityOfService = NULL;                                                                          \
    }

/**
 * @brief Represents a Unicode string in the NT environment.
 */
typedef struct _SND_UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} SND_UNICODE_STRING, *PSND_UNICODE_STRING;

typedef struct _SND_OBJECT_ATTRIBUTES {
    ULONG               Length;
    HANDLE              RootDirectory;
    PSND_UNICODE_STRING ObjectName;
    ULONG               Attributes;
    PVOID               SecurityDescriptor;
    PVOID               SecurityQualityOfService;
} SND_OBJECT_ATTRIBUTES, *PSND_OBJECT_ATTRIBUTES;

/**
 * @brief Client ID structure.
 */
typedef struct _SND_CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} SND_CLIENT_ID, *PSND_CLIENT_ID;

/**
 * @brief Initializes a UNICODE_STRING structure from a wide string buffer and length.
 * Replaces RtlInitUnicodeString.
 */
SND_FORCE_INLINE void snd_init_unicode_string(SND_UNICODE_STRING *us, const wchar_t *buf, size_t char_count) {
    if (us) {
        us->Buffer        = (wchar_t *)buf;
        us->Length        = (USHORT)(char_count * sizeof(wchar_t));
        us->MaximumLength = us->Length + sizeof(wchar_t);
    }
}

SND_END_EXTERN_C

#endif // SND_INTERNAL_NT_TYPES_H
