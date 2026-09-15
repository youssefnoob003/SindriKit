#ifndef SND_INTERNAL_WINDOWS_TYPES_H
#define SND_INTERNAL_WINDOWS_TYPES_H

#include <stdint.h>

#if defined(SND_USE_WINDOWS_SDK)
#include <windows.h>
#else
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#ifndef NTAPI
#define NTAPI __stdcall
#endif
typedef void           VOID;
typedef char           CHAR;
typedef short          SHORT, *PSHORT;
typedef unsigned long  DWORD, *PDWORD;
typedef int            BOOL, *PBOOL;
typedef unsigned char  BYTE, *PBYTE;
typedef unsigned short WORD, *PWORD;
typedef int            INT, *PINT;
typedef int32_t        INT32;
typedef int32_t        LONG32;
typedef uint64_t       ULONG64;
typedef unsigned int   UINT, *PUINT;
typedef long           LONG, *PLONG;
typedef unsigned long  ULONG, *PULONG;
typedef unsigned short USHORT, *PUSHORT;
typedef unsigned char  UCHAR, *PUCHAR;
typedef char          *PSZ;
typedef void          *PVOID;
typedef void          *LPVOID;
typedef PVOID          HANDLE;
typedef HANDLE         HINSTANCE;
typedef HANDLE         HMODULE;
typedef HANDLE        *PHANDLE;
#if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
typedef uint64_t ULONG_PTR, *PULONG_PTR;
typedef uint64_t UINT_PTR, *PUINT_PTR;
typedef int64_t  LONG_PTR, *PLONG_PTR;
typedef int64_t  INT_PTR, *PINT_PTR;
typedef uint64_t SIZE_T, *PSIZE_T;
#else
typedef uint32_t ULONG_PTR, *PULONG_PTR;
typedef uint32_t UINT_PTR, *PUINT_PTR;
typedef int32_t  LONG_PTR, *PLONG_PTR;
typedef int32_t  INT_PTR, *PINT_PTR;
typedef uint32_t SIZE_T, *PSIZE_T;
#endif
typedef void          *FARPROC;
typedef uint64_t       ULONGLONG, *PULONGLONG;
typedef int64_t        LONGLONG, *PLONGLONG;
typedef wchar_t       *PWSTR;
typedef const wchar_t *PCWSTR;
typedef char          *PSTR;
typedef const char    *PCSTR;
typedef const char    *LPCSTR;
typedef int            NTSTATUS;
typedef long           ACCESS_MASK;
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR) - 1)
#endif
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

typedef union _SND_LARGE_INTEGER {
    struct {
        DWORD LowPart;
        LONG  HighPart;
    } u;
    LONGLONG QuadPart;
} SND_LARGE_INTEGER, *PSND_LARGE_INTEGER;

typedef union _SND_ULARGE_INTEGER {
    struct {
        DWORD LowPart;
        DWORD HighPart;
    } u;
    ULONGLONG QuadPart;
} SND_ULARGE_INTEGER, *PSND_ULARGE_INTEGER;

#define SND_MAX_PATH  260
#define SND_MAX_WORD  ((WORD)0xffff)
#define SND_MAX_DWORD ((DWORD)0xffffffffUL)

#endif /* SND_INTERNAL_WINDOWS_TYPES_H */
