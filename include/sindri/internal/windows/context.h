#ifndef SND_INTERNAL_WINDOWS_CONTEXT_H
#define SND_INTERNAL_WINDOWS_CONTEXT_H

#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <sindri/internal/windows/types.h>
#include <sindri/primitives/thread.h>
#include <stddef.h>

#if defined(_WIN64)

/**
 * @name Context Architecture Flags
 * @{
 */
#define SND_CONTEXT_ARCH_FLAGS 0x00100003UL /* CONTEXT_AMD64 | CONTROL | INTEGER */
/** @} */

/**
 * @brief WINNT CONTEXT (AMD64) Mirror
 */
typedef struct SND_ALIGNED(16) _SND_CONTEXT_AMD64 {
    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5Home;
    ULONG64 P6Home;
    DWORD   ContextFlags;
    DWORD   MxCsr;
    WORD    SegCs;
    WORD    SegDs;
    WORD    SegEs;
    WORD    SegFs;
    WORD    SegGs;
    WORD    SegSs;
    DWORD   EFlags;
    ULONG64 Dr0;
    ULONG64 Dr1;
    ULONG64 Dr2;
    ULONG64 Dr3;
    ULONG64 Dr6;
    ULONG64 Dr7;
    ULONG64 Rax;
    ULONG64 Rcx;
    ULONG64 Rdx;
    ULONG64 Rbx;
    ULONG64 Rsp;
    ULONG64 Rbp;
    ULONG64 Rsi;
    ULONG64 Rdi;
    ULONG64 R8;
    ULONG64 R9;
    ULONG64 R10;
    ULONG64 R11;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;
    ULONG64 Rip;
    BYTE    Tail[0x3D0];
} SND_CONTEXT_AMD64;

typedef SND_CONTEXT_AMD64 SND_NATIVE_CONTEXT;

#define SND_CTX_FLD_IP    Rip
#define SND_CTX_FLD_SP    Rsp
#define SND_CTX_FLD_CX    Rcx
#define SND_CTX_FLD_DX    Rdx
#define SND_CTX_FLD_FLAGS EFlags

#else                                       /* x86 */

/**
 * @name Context Architecture Flags
 * @{
 */
#define SND_CONTEXT_ARCH_FLAGS 0x00010003UL /* CONTEXT_i386 | CONTROL | INTEGER  */
/** @} */

/**
 * @brief WINNT CONTEXT (i386) Mirror
 */
typedef struct _SND_FLOATING_SAVE_AREA {
    DWORD ControlWord;
    DWORD StatusWord;
    DWORD TagWord;
    DWORD ErrorOffset;
    DWORD ErrorSelector;
    DWORD DataOffset;
    DWORD DataSelector;
    BYTE  RegisterArea[80];
    DWORD Cr0NpxState;
} SND_FLOATING_SAVE_AREA;

typedef struct _SND_CONTEXT_I386 {
    DWORD                  ContextFlags;
    DWORD                  Dr0;
    DWORD                  Dr1;
    DWORD                  Dr2;
    DWORD                  Dr3;
    DWORD                  Dr6;
    DWORD                  Dr7;
    SND_FLOATING_SAVE_AREA FloatSave;
    DWORD                  SegGs;
    DWORD                  SegFs;
    DWORD                  SegEs;
    DWORD                  SegDs;
    DWORD                  Edi;
    DWORD                  Esi;
    DWORD                  Ebx;
    DWORD                  Edx;
    DWORD                  Ecx;
    DWORD                  Eax;
    DWORD                  Ebp;
    DWORD                  Eip;
    DWORD                  SegCs;
    DWORD                  EFlags;
    DWORD                  Esp;
    DWORD                  SegSs;
    BYTE                   ExtendedRegisters[512];
} SND_CONTEXT_I386;

typedef SND_CONTEXT_I386 SND_NATIVE_CONTEXT;

#define SND_CTX_FLD_IP    Eip
#define SND_CTX_FLD_SP    Esp
#define SND_CTX_FLD_CX    Ecx
#define SND_CTX_FLD_DX    Edx
#define SND_CTX_FLD_FLAGS EFlags

#endif /* arch */

/**
 * @name Native Field Offsets
 * @brief Compile-time constants for field offsets.
 * @{
 */
#define SND_CTX_OFF_IP    offsetof(SND_NATIVE_CONTEXT, SND_CTX_FLD_IP)
#define SND_CTX_OFF_SP    offsetof(SND_NATIVE_CONTEXT, SND_CTX_FLD_SP)
#define SND_CTX_OFF_CX    offsetof(SND_NATIVE_CONTEXT, SND_CTX_FLD_CX)
#define SND_CTX_OFF_DX    offsetof(SND_NATIVE_CONTEXT, SND_CTX_FLD_DX)
#define SND_CTX_OFF_FLAGS offsetof(SND_NATIVE_CONTEXT, SND_CTX_FLD_FLAGS)
/** @} */

/**
 * @brief Copies the projected registers out of a native CONTEXT buffer.
 * @param native Pointer to a native context (SDK CONTEXT or mirror) laid out
 *        identically to SND_NATIVE_CONTEXT.
 * @param out    Receives the portable projection.
 */
SND_FORCE_INLINE void snd_context_to_portable(const void *native, SND_THREAD_REGISTERS *out) {
    out->ip     = *(const ULONG_PTR *)SND_PTR_ADD(native, SND_CTX_OFF_IP);
    out->sp     = *(const ULONG_PTR *)SND_PTR_ADD(native, SND_CTX_OFF_SP);
    out->cx     = *(const ULONG_PTR *)SND_PTR_ADD(native, SND_CTX_OFF_CX);
    out->dx     = *(const ULONG_PTR *)SND_PTR_ADD(native, SND_CTX_OFF_DX);
    out->rflags = *(const DWORD *)SND_PTR_ADD(native, SND_CTX_OFF_FLAGS);
}

/**
 * @brief Writes the projected registers into a native CONTEXT buffer.
 * @param in     Portable projection to apply.
 * @param native Pointer to a zero-initialized native context buffer.
 */
SND_FORCE_INLINE void snd_context_from_portable(const SND_THREAD_REGISTERS *in, void *native) {
    *(ULONG_PTR *)SND_PTR_ADD(native, SND_CTX_OFF_IP) = in->ip;
    *(ULONG_PTR *)SND_PTR_ADD(native, SND_CTX_OFF_SP) = in->sp;
    *(ULONG_PTR *)SND_PTR_ADD(native, SND_CTX_OFF_CX) = in->cx;
    *(ULONG_PTR *)SND_PTR_ADD(native, SND_CTX_OFF_DX) = in->dx;
    *(DWORD *)SND_PTR_ADD(native, SND_CTX_OFF_FLAGS)  = in->rflags;
}

#endif /* SND_INTERNAL_WINDOWS_CONTEXT_H */
