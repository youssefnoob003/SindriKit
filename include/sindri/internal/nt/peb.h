#ifndef SND_INTERNAL_NT_PEB_H
#define SND_INTERNAL_NT_PEB_H

#include <sindri/common/macros.h>
#include <sindri/internal/nt/base.h>

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
    ULONG          Length;
    BYTE           Reserved[8];
    SND_LIST_ENTRY InLoadOrderModuleList;
    SND_LIST_ENTRY InMemoryOrderModuleList;
    SND_LIST_ENTRY InInitializationOrderModuleList;
} SND_PEB_LDR_DATA, *PSND_PEB_LDR_DATA;

/**
 * @brief Loader data table entry for module tracking.
 */
typedef struct _SND_LDR_DATA_TABLE_ENTRY {
    SND_LIST_ENTRY     InLoadOrderLinks;
    SND_LIST_ENTRY     InMemoryOrderLinks;
    SND_LIST_ENTRY     InInitializationOrderLinks;
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

typedef struct _SND_RTL_USER_PROCESS_PARAMETERS_FULL {
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
    ULONG              StartingX;
    ULONG              StartingY;
    ULONG              CountX;
    ULONG              CountY;
    ULONG              CountCharsX;
    ULONG              CountCharsY;
    ULONG              FillAttribute;
    ULONG              WindowFlags;
    ULONG              ShowWindowFlags;
    SND_UNICODE_STRING WindowTitle;
    SND_UNICODE_STRING DesktopInfo;
    SND_UNICODE_STRING ShellInfo;
    SND_UNICODE_STRING RuntimeData;
    BYTE               Reserved[0x350];
} SND_RTL_USER_PROCESS_PARAMETERS_FULL, *PSND_RTL_USER_PROCESS_PARAMETERS_FULL;

// Offset of the undocumented EnvironmentSize field in RTL_USER_PROCESS_PARAMETERS.
// This differs between architectures because the struct contains pointer-sized members.
#ifdef _WIN64
#define SND_ENVSIZE_OFFSET 0x3F0
#else
#define SND_ENVSIZE_OFFSET 0x0290
#endif

SND_END_EXTERN_C

#endif // SND_INTERNAL_NT_PEB_H
