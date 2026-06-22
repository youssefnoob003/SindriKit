#ifndef SND_INTERNAL_NT_DEFS_H
#define SND_INTERNAL_NT_DEFS_H
#include <sindri/common/helpers.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Macro to check if an NTSTATUS indicates success.
 */
#ifndef NT_SUCCESS
#define NT_SUCCESS(s) (((NTSTATUS)(s)) >= 0)
#endif

#define SND_OBJ_CASE_INSENSITIVE 0x00000040L
#define SND_PAGE_SIZE            ((SIZE_T)0x1000)

/**
 * @brief Represents a Unicode string in the NT environment.
 */
typedef struct _SND_UNICODE_STRING {
    USHORT Length;        // In BYTES
    USHORT MaximumLength; // In BYTES
    PWSTR  Buffer;
} SND_UNICODE_STRING, *PSND_UNICODE_STRING;

/**
 * @brief PEB loader data structure.
 */
typedef struct _SND_PEB_LDR_DATA {
    ULONG      Length;
    BYTE       Reserved[8];
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} SND_PEB_LDR_DATA, *PSND_PEB_LDR_DATA;

/**
 * @brief Process Environment Block (PEB) definition.
 */
typedef struct _SND_PEB {
    BYTE              Reserved1[2];
    BYTE              BeingDebugged;
    BYTE              Reserved2[1];
    PVOID             Reserved3[2];
    PSND_PEB_LDR_DATA Ldr;
} SND_PEB, *PSND_PEB;

/**
 * @brief Loader data table entry for module tracking.
 */
typedef struct _SND_LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY         InLoadOrderLinks;
    LIST_ENTRY         InMemoryOrderLinks;
    LIST_ENTRY         InInitializationOrderLinks;
    PVOID              DllBase;
    PVOID              EntryPoint;
    ULONG              SizeOfImage;
    SND_UNICODE_STRING FullDllName;
    SND_UNICODE_STRING BaseDllName;
} SND_LDR_DATA_TABLE_ENTRY, *PSND_LDR_DATA_TABLE_ENTRY;

/**
 * @brief Type definition for LdrLoadDll function (ntdll native loader, no
 * kernel32 required).
 */
typedef NTSTATUS(NTAPI *LdrLoadDll_t)(PWSTR               PathToFile, // optional search path (NULL = default)
                                      PULONG              Flags,      // optional flags   (NULL = 0)
                                      PSND_UNICODE_STRING ModuleFileName, PHANDLE ModuleHandle);

typedef struct _SND_OBJECT_ATTRIBUTES {
    ULONG               Length;
    HANDLE              RootDirectory;
    PSND_UNICODE_STRING ObjectName;
    ULONG               Attributes;
    PVOID               SecurityDescriptor;
    PVOID               SecurityQualityOfService;
} SND_OBJECT_ATTRIBUTES, *PSND_OBJECT_ATTRIBUTES;

#define SND_InitializeObjectAttributes(p, n, a, r, s)                                                                  \
    {                                                                                                                  \
        (p)->Length                   = sizeof(SND_OBJECT_ATTRIBUTES);                                                 \
        (p)->RootDirectory            = r;                                                                             \
        (p)->Attributes               = a;                                                                             \
        (p)->ObjectName               = n;                                                                             \
        (p)->SecurityDescriptor       = s;                                                                             \
        (p)->SecurityQualityOfService = NULL;                                                                          \
    }

SND_END_EXTERN_C

#endif // SND_INTERNAL_NT_DEFS_H
