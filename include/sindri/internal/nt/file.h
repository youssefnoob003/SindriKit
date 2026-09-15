#ifndef SND_INTERNAL_NT_FILE_H
#define SND_INTERNAL_NT_FILE_H

#include <sindri/internal/nt/base.h>

#define SND_SYNCHRONIZE                     0x00100000UL
#define SND_FILE_OPEN                       0x00000001UL
#define SND_FILE_NON_DIRECTORY_FILE         0x00000040UL
#define SND_FILE_SYNCHRONOUS_IO_NONALERT    0x00000020UL
#define SND_FILE_STANDARD_INFORMATION_CLASS 5

typedef struct {
    union {
        NTSTATUS Status;
        PVOID    Pointer;
    } u;
    ULONG_PTR Information;
} SND_IO_STATUS_BLOCK;

typedef struct {
    SND_LARGE_INTEGER AllocationSize;
    SND_LARGE_INTEGER EndOfFile;
    ULONG             NumberOfLinks;
    BYTE              DeletePending;
    BYTE              Directory;
} SND_FILE_STANDARD_INFORMATION;

#endif // SND_INTERNAL_NT_FILE_H
