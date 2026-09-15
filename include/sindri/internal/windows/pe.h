#ifndef SND_INTERNAL_WINDOWS_PE_H
#define SND_INTERNAL_WINDOWS_PE_H

#include <sindri/internal/windows/image.h>

#define SND_IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16
#define SND_IMAGE_NT_OPTIONAL_HDR32_MAGIC    0x10b
#define SND_IMAGE_NT_OPTIONAL_HDR64_MAGIC    0x20b
#define SND_IMAGE_FILE_RELOCS_STRIPPED       0x0001
#define SND_IMAGE_FILE_DLL                   0x2000

// Optional Header 32
typedef struct _SND_IMAGE_OPTIONAL_HEADER32 {
    WORD                     Magic;
    BYTE                     MajorLinkerVersion;
    BYTE                     MinorLinkerVersion;
    DWORD                    SizeOfCode;
    DWORD                    SizeOfInitializedData;
    DWORD                    SizeOfUninitializedData;
    DWORD                    AddressOfEntryPoint;
    DWORD                    BaseOfCode;
    DWORD                    BaseOfData;
    DWORD                    ImageBase;
    DWORD                    SectionAlignment;
    DWORD                    FileAlignment;
    WORD                     MajorOperatingSystemVersion;
    WORD                     MinorOperatingSystemVersion;
    WORD                     MajorImageVersion;
    WORD                     MinorImageVersion;
    WORD                     MajorSubsystemVersion;
    WORD                     MinorSubsystemVersion;
    DWORD                    Win32VersionValue;
    DWORD                    SizeOfImage;
    DWORD                    SizeOfHeaders;
    DWORD                    CheckSum;
    WORD                     Subsystem;
    WORD                     DllCharacteristics;
    DWORD                    SizeOfStackReserve;
    DWORD                    SizeOfStackCommit;
    DWORD                    SizeOfHeapReserve;
    DWORD                    SizeOfHeapCommit;
    DWORD                    LoaderFlags;
    DWORD                    NumberOfRvaAndSizes;
    SND_IMAGE_DATA_DIRECTORY DataDirectory[SND_IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} SND_IMAGE_OPTIONAL_HEADER32, *PSND_IMAGE_OPTIONAL_HEADER32;

// Optional Header 64
typedef struct _SND_IMAGE_OPTIONAL_HEADER64 {
    WORD                     Magic;
    BYTE                     MajorLinkerVersion;
    BYTE                     MinorLinkerVersion;
    DWORD                    SizeOfCode;
    DWORD                    SizeOfInitializedData;
    DWORD                    SizeOfUninitializedData;
    DWORD                    AddressOfEntryPoint;
    DWORD                    BaseOfCode;
    ULONGLONG                ImageBase;
    DWORD                    SectionAlignment;
    DWORD                    FileAlignment;
    WORD                     MajorOperatingSystemVersion;
    WORD                     MinorOperatingSystemVersion;
    WORD                     MajorImageVersion;
    WORD                     MinorImageVersion;
    WORD                     MajorSubsystemVersion;
    WORD                     MinorSubsystemVersion;
    DWORD                    Win32VersionValue;
    DWORD                    SizeOfImage;
    DWORD                    SizeOfHeaders;
    DWORD                    CheckSum;
    WORD                     Subsystem;
    WORD                     DllCharacteristics;
    ULONGLONG                SizeOfStackReserve;
    ULONGLONG                SizeOfStackCommit;
    ULONGLONG                SizeOfHeapReserve;
    ULONGLONG                SizeOfHeapCommit;
    DWORD                    LoaderFlags;
    DWORD                    NumberOfRvaAndSizes;
    SND_IMAGE_DATA_DIRECTORY DataDirectory[SND_IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} SND_IMAGE_OPTIONAL_HEADER64, *PSND_IMAGE_OPTIONAL_HEADER64;

// NT Headers
typedef struct _SND_IMAGE_NT_HEADERS64 {
    DWORD                       Signature;
    SND_IMAGE_FILE_HEADER       FileHeader;
    SND_IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} SND_IMAGE_NT_HEADERS64, *PSND_IMAGE_NT_HEADERS64;

typedef struct _SND_IMAGE_NT_HEADERS32 {
    DWORD                       Signature;
    SND_IMAGE_FILE_HEADER       FileHeader;
    SND_IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} SND_IMAGE_NT_HEADERS32, *PSND_IMAGE_NT_HEADERS32;

// Directories
#define SND_IMAGE_DIRECTORY_ENTRY_EXPORT         0
#define SND_IMAGE_DIRECTORY_ENTRY_IMPORT         1
#define SND_IMAGE_DIRECTORY_ENTRY_RESOURCE       2
#define SND_IMAGE_DIRECTORY_ENTRY_EXCEPTION      3
#define SND_IMAGE_DIRECTORY_ENTRY_SECURITY       4
#define SND_IMAGE_DIRECTORY_ENTRY_BASERELOC      5
#define SND_IMAGE_DIRECTORY_ENTRY_DEBUG          6
#define SND_IMAGE_DIRECTORY_ENTRY_ARCHITECTURE   7
#define SND_IMAGE_DIRECTORY_ENTRY_GLOBALPTR      8
#define SND_IMAGE_DIRECTORY_ENTRY_TLS            9
#define SND_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10
#define SND_IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11
#define SND_IMAGE_DIRECTORY_ENTRY_IAT            12
#define SND_IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13
#define SND_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14

// Export Directory
typedef struct _SND_IMAGE_EXPORT_DIRECTORY {
    DWORD Characteristics;
    DWORD TimeDateStamp;
    WORD  MajorVersion;
    WORD  MinorVersion;
    DWORD Name;
    DWORD Base;
    DWORD NumberOfFunctions;
    DWORD NumberOfNames;
    DWORD AddressOfFunctions;
    DWORD AddressOfNames;
    DWORD AddressOfNameOrdinals;
} SND_IMAGE_EXPORT_DIRECTORY, *PSND_IMAGE_EXPORT_DIRECTORY;

// Import Descriptor
typedef struct _SND_IMAGE_IMPORT_DESCRIPTOR {
    union {
        DWORD Characteristics;
        DWORD OriginalFirstThunk;
    } u;
    DWORD TimeDateStamp;
    DWORD ForwarderChain;
    DWORD Name;
    DWORD FirstThunk;
} SND_IMAGE_IMPORT_DESCRIPTOR, *PSND_IMAGE_IMPORT_DESCRIPTOR;

// Thunks
typedef struct _SND_IMAGE_THUNK_DATA64 {
    union {
        ULONGLONG ForwarderString;
        ULONGLONG Function;
        ULONGLONG Ordinal;
        ULONGLONG AddressOfData;
    } u1;
} SND_IMAGE_THUNK_DATA64, *PSND_IMAGE_THUNK_DATA64;

typedef struct _SND_IMAGE_THUNK_DATA32 {
    union {
        DWORD ForwarderString;
        DWORD Function;
        DWORD Ordinal;
        DWORD AddressOfData;
    } u1;
} SND_IMAGE_THUNK_DATA32, *PSND_IMAGE_THUNK_DATA32;

#define SND_IMAGE_ORDINAL_FLAG64             0x8000000000000000ull
#define SND_IMAGE_ORDINAL_FLAG32             0x80000000
#define SND_IMAGE_ORDINAL64(Ordinal)         (Ordinal & 0xffff)
#define SND_IMAGE_ORDINAL32(Ordinal)         (Ordinal & 0xffff)
#define SND_IMAGE_SNAP_BY_ORDINAL64(Ordinal) ((Ordinal & SND_IMAGE_ORDINAL_FLAG64) != 0)
#define SND_IMAGE_SNAP_BY_ORDINAL32(Ordinal) ((Ordinal & SND_IMAGE_ORDINAL_FLAG32) != 0)
#define SND_IS_INTRESOURCE(value)            (((ULONG_PTR)(value) >> 16) == 0)
#define SND_MAKEINTRESOURCE(value)           ((const char *)(ULONG_PTR)(value))

typedef struct _SND_IMAGE_IMPORT_BY_NAME {
    WORD Hint;
    CHAR Name[1];
} SND_IMAGE_IMPORT_BY_NAME, *PSND_IMAGE_IMPORT_BY_NAME;

// Base Relocation
typedef struct _SND_IMAGE_BASE_RELOCATION {
    DWORD VirtualAddress;
    DWORD SizeOfBlock;
} SND_IMAGE_BASE_RELOCATION, *PSND_IMAGE_BASE_RELOCATION;

#define SND_IMAGE_REL_BASED_ABSOLUTE 0
#define SND_IMAGE_REL_BASED_HIGH     1
#define SND_IMAGE_REL_BASED_LOW      2
#define SND_IMAGE_REL_BASED_HIGHLOW  3
#define SND_IMAGE_REL_BASED_HIGHADJ  4
#define SND_IMAGE_REL_BASED_DIR64    10

// TLS Directory
typedef struct _SND_IMAGE_TLS_DIRECTORY64 {
    ULONGLONG StartAddressOfRawData;
    ULONGLONG EndAddressOfRawData;
    ULONGLONG AddressOfIndex;
    ULONGLONG AddressOfCallBacks;
    DWORD     SizeOfZeroFill;
    union {
        DWORD Characteristics;
        struct {
            DWORD Reserved0 : 20;
            DWORD Alignment : 4;
            DWORD Reserved1 : 8;
        } bits;
    } u;
} SND_IMAGE_TLS_DIRECTORY64, *PSND_IMAGE_TLS_DIRECTORY64;

typedef struct _SND_IMAGE_TLS_DIRECTORY32 {
    DWORD StartAddressOfRawData;
    DWORD EndAddressOfRawData;
    DWORD AddressOfIndex;
    DWORD AddressOfCallBacks;
    DWORD SizeOfZeroFill;
    union {
        DWORD Characteristics;
        struct {
            DWORD Reserved0 : 20;
            DWORD Alignment : 4;
            DWORD Reserved1 : 8;
        } bits;
    } u;
} SND_IMAGE_TLS_DIRECTORY32, *PSND_IMAGE_TLS_DIRECTORY32;

typedef VOID(NTAPI *SND_PIMAGE_TLS_CALLBACK)(PVOID DllHandle, DWORD Reason, PVOID Reserved);

// Runtime Function
typedef struct _SND_IMAGE_RUNTIME_FUNCTION_ENTRY {
    DWORD BeginAddress;
    DWORD EndAddress;
    union {
        DWORD UnwindInfoAddress;
        DWORD UnwindData;
    } u;
} SND_IMAGE_RUNTIME_FUNCTION_ENTRY, *PSND_IMAGE_RUNTIME_FUNCTION_ENTRY;

typedef SND_IMAGE_RUNTIME_FUNCTION_ENTRY SND_IMAGE_RUNTIME_FUNCTION, *PSND_IMAGE_RUNTIME_FUNCTION;

#endif // SND_INTERNAL_WINDOWS_PE_H
