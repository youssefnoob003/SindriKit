#ifndef SND_INTERNAL_NT_PROCESS_H
#define SND_INTERNAL_NT_PROCESS_H

#include <sindri/common/macros.h>
#include <sindri/internal/nt/base.h>

SND_BEGIN_EXTERN_C

#define SND_CURRENT_PROCESS ((HANDLE)(LONG_PTR) - 1)

#define SND_PROCESS_CREATE_FLAGS_SUSPENDED       0x00000200
#define SND_THREAD_CREATE_FLAGS_CREATE_SUSPENDED 0x00000001
#define SND_PROCESS_ALL_ACCESS                   0x001FFFFF
#define SND_THREAD_ALL_ACCESS                    0x001FFFFF

#define SND_PS_ATTRIBUTE_NUMBER_MASK 0x0000FFFF
#define SND_PS_ATTRIBUTE_THREAD      0x00010000
#define SND_PS_ATTRIBUTE_INPUT       0x00020000
#define SND_PS_ATTRIBUTE_ADDITIVE    0x00040000
#define SND_PS_ATTRIBUTE_IMAGE_NAME  (0x05 | SND_PS_ATTRIBUTE_INPUT)

typedef enum _SND_PS_CREATE_STATE {
    SND_PS_CREATE_INITIAL_STATE,
    SND_PS_CREATE_FAIL_ON_FILE_OPEN,
    SND_PS_CREATE_FAIL_ON_SECTION_CREATE,
    SND_PS_CREATE_FAIL_EXE_FORMAT,
    SND_PS_CREATE_FAIL_MACHINE_MISMATCH,
    SND_PS_CREATE_FAIL_EXE_NAME,
    SND_PS_CREATE_SUCCESS,
    SND_PS_CREATE_MAXIMUM_STATES
} SND_PS_CREATE_STATE,
    *PSND_PS_CREATE_STATE;

typedef struct _SND_PS_ATTRIBUTE {
    ULONG_PTR Attribute;
    SIZE_T    Size;
    union {
        ULONG_PTR Value;
        PVOID     ValuePtr;
    } ValueUnion;
    PSIZE_T ReturnLength;
} SND_PS_ATTRIBUTE, *PSND_PS_ATTRIBUTE;

typedef struct _SND_PS_ATTRIBUTE_LIST {
    SIZE_T           TotalLength;
    SND_PS_ATTRIBUTE Attributes[1];
} SND_PS_ATTRIBUTE_LIST, *PSND_PS_ATTRIBUTE_LIST;

typedef struct _SND_PS_CREATE_INFO {
    SIZE_T              Size;
    SND_PS_CREATE_STATE State;
    union {
        struct {
            union {
                ULONG InitFlags;
                struct {
                    UCHAR  WriteOutputOnExit : 1;
                    UCHAR  DetectManifest : 1;
                    UCHAR  IFEOSkipDebugger : 1;
                    UCHAR  IFEODoNotPropagateKeyState : 1;
                    UCHAR  SpareBits1 : 4;
                    UCHAR  SpareBits2 : 8;
                    USHORT ProhibitedImageCharacteristics : 16;
                } InitBits;
            } InitFlagsUnion;
            ACCESS_MASK AdditionalFileAccess;
        } InitState;
        struct {
            HANDLE FileHandle;
        } FailSection;
        struct {
            USHORT DllCharacteristics;
        } ExeFormat;
        struct {
            HANDLE IFEOKey;
        } ExeName;
        struct {
            union {
                ULONG OutputFlags;
                struct {
                    UCHAR  ProtectedProcess : 1;
                    UCHAR  AddressSpaceOverride : 1;
                    UCHAR  DevOverrideEnabled : 1;
                    UCHAR  ManifestDetected : 1;
                    UCHAR  ProtectedProcessLight : 1;
                    UCHAR  SpareBits1 : 3;
                    UCHAR  SpareBits2 : 8;
                    USHORT SpareBits3 : 16;
                } SuccessBits;
            } OutputFlagsUnion;
            HANDLE    FileHandle;
            HANDLE    SectionHandle;
            ULONGLONG UserProcessParametersNative;
            ULONG     UserProcessParametersWow64;
            ULONG     CurrentParameterFlags;
            ULONGLONG PebAddressNative;
            ULONG     PebAddressWow64;
            ULONGLONG ManifestAddress;
            ULONG     ManifestSize;
            ULONG     PackageBreakaway;
        } SuccessState;
    } StateUnion;
} SND_PS_CREATE_INFO, *PSND_PS_CREATE_INFO;

SND_END_EXTERN_C

#endif /* SND_INTERNAL_NT_PROCESS_H */
