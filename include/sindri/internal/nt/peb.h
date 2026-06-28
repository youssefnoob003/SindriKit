#ifndef SND_INTERNAL_NT_DEFS_H
#define SND_INTERNAL_NT_DEFS_H

#include <sindri/common/macros.h>
#include <sindri/internal/nt/types.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief A basic representation of the current directory struct to align memory block layouts.
 */
typedef struct _SND_CURDIR {
    SND_UNICODE_STRING DosPath;
    HANDLE             Handle;
} SND_CURDIR, *PSND_CURDIR;

/**
 * @brief Process parameters structure containing environment configuration details.
 */
typedef struct _SND_RTL_USER_PROCESS_PARAMETERS {
    ULONG              MaximumLength;
    ULONG              Length;
    ULONG              Flags;
    ULONG              DebugFlags;
    HANDLE             ConsoleHandle;
    ULONG              ConsoleFlags;
    HANDLE             StandardInput;
    HANDLE             StandardOutput;
    HANDLE             StandardError;
    SND_CURDIR         CurrentDirectory;
    SND_UNICODE_STRING DllPath;
    SND_UNICODE_STRING ImagePathName;
    SND_UNICODE_STRING CommandLine;
    PVOID              Environment;
} SND_RTL_USER_PROCESS_PARAMETERS, *PSND_RTL_USER_PROCESS_PARAMETERS;

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
 * @brief Process Environment Block (PEB) definition.
 */
typedef struct _SND_PEB {
    BYTE                             Reserved1[2];
    BYTE                             BeingDebugged;
    BYTE                             Reserved2[1];
    PVOID                            Reserved3[2];
    PSND_PEB_LDR_DATA                Ldr;
    PSND_RTL_USER_PROCESS_PARAMETERS ProcessParameters;
} SND_PEB, *PSND_PEB;

SND_END_EXTERN_C

#endif // SND_INTERNAL_NT_DEFS_H
