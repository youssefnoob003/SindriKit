#ifndef SND_INTERNAL_NT_BASE_H
#define SND_INTERNAL_NT_BASE_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/types.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Macro to check if an NTSTATUS indicates success.
 */
#define SND_NT_SUCCESS(s) (((NTSTATUS)(s)) >= 0)

/**
 * @brief Macro to check if an NTSTATUS indicates failure.
 */
#define SND_NT_FAILURE(s) (((NTSTATUS)(s)) < 0)

/**
 * @brief Object attribute flag indicating case-insensitive object names.
 */
#define SND_OBJ_CASE_INSENSITIVE 0x00000040L

/**
 * @brief Address of KUSER_SHARED_DATA->SystemTime.LowPart.
 * Used as a source of entropy that is updated every tick.
 */
#define SND_KUSER_SHARED_DATA_SYSTEM_TIME 0x7FFE0014

/**
 * @brief Page size used in memory allocations.
 */
#define SND_UNICODE_STRING_MAX_CHARS 32768
#define SND_SECTION_MAP_READ         0x0004

/**
 * @brief Represents a Unicode string in the NT environment.
 */
typedef struct _SND_UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} SND_UNICODE_STRING, *PSND_UNICODE_STRING;

typedef struct _SND_LIST_ENTRY {
    struct _SND_LIST_ENTRY *Flink;
    struct _SND_LIST_ENTRY *Blink;
} SND_LIST_ENTRY, *PSND_LIST_ENTRY;

/**
 * @brief Represents object attributes.
 */
typedef struct _SND_OBJECT_ATTRIBUTES {
    ULONG               Length;
    HANDLE              RootDirectory;
    PSND_UNICODE_STRING ObjectName;
    ULONG               Attributes;
    PVOID               SecurityDescriptor;
    PVOID               SecurityQualityOfService;
} SND_OBJECT_ATTRIBUTES, *PSND_OBJECT_ATTRIBUTES;

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
 * @brief Client ID structure.
 */
typedef struct _SND_CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} SND_CLIENT_ID, *PSND_CLIENT_ID;

/**
 * @brief Initializes a UNICODE_STRING structure from a wide string buffer and length.
 */
SND_FORCE_INLINE void snd_init_unicode_string(SND_UNICODE_STRING *us, const wchar_t *buf, size_t char_count) {
    if (us) {
        us->Buffer        = (wchar_t *)buf;
        us->Length        = (USHORT)(char_count * sizeof(wchar_t));
        us->MaximumLength = us->Length + sizeof(wchar_t);
    }
}

SND_END_EXTERN_C

#endif // SND_INTERNAL_NT_BASE_H
