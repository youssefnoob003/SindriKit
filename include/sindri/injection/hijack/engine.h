#ifndef SND_INJECTION_HIJACK_ENGINE_H
#define SND_INJECTION_HIJACK_ENGINE_H

#include <sindri/common/macros.h>
#include <sindri/injection/context.h>
#include <sindri/internal/windows/context.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Creates a target process in a suspended state for hijack injection.
 *
 * The returned initial thread handle (in @p ctx::remote_thread) is what hijack
 * rewrites.
 *
 * @param ctx Initialized injection context with target_image_path and proc_api set.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, the process API, or its
 * `create_process` callback is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the injection stage is not
 * `SND_INJ_STAGE_UNINITIALIZED`.
 * @retval SND_STATUS_CORRUPTED_STAGE If `target_image_path` is not set.
 * @retval Any error returned by `snd_process_api_t::create_process`.
 */
snd_status_t snd_inj_hijack_create_target(snd_inj_ctx_t *ctx);

/**
 * @name Entry-Frame ABI Policy
 * @brief Architecture-specific rules for painting a thread's entry frame.
 * @{
 */
#define SND_EFLAGS_IF           0x00000200UL
#define SND_EFLAGS_RESERVED1    0x00000002UL
#define SND_ENTRY_FLAGS(rflags) (((rflags) & SND_EFLAGS_IF) | SND_EFLAGS_RESERVED1)

#if defined(_WIN64)
#define SND_ENTRY_REGISTER_ARGS 1 /* first args ride in RCX/RDX         */
#define SND_ENTRY_ALIGN_SP(sp)  (((sp) & ~((ULONG_PTR)0xF)) - sizeof(PVOID))
#else
#define SND_ENTRY_REGISTER_ARGS 0    /* cdecl/stdcall marshal args on stack */
#define SND_ENTRY_ALIGN_SP(sp)  (sp) /* no alignment requirement on x86     */
#endif
/** @} */

/**
 * @brief Paints a remote entry frame from intent onto a portable context.
 *
 * Pure function: no I/O. Places @p entry into `ip`, aligns `sp` from the live
 * context via the arch ABI macro, masks `rflags`, and — only when the
 * architecture marshals arguments in registers — wires `cx := arg1`,
 * `dx := arg2`.
 *
 * @param live Live context captured from the suspended thread (ip unread;
 *        sp and rflags consumed).
 * @param entry Target instruction pointer (payload entry point, never NULL).
 * @param arg1  First argument (BOF args pointer, else 0/NULL).
 * @param arg2  Second argument (BOF arg_len, else 0).
 * @param out   Receives the painted frame to pass to `set_context`.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p live, @p out, or @p entry is NULL.
 * @retval SND_STATUS_ARCH_MISMATCH On architectures that marshal entry
 * arguments on the stack (x86 cdecl/stdcall), which a context-only hijack
 * cannot express.
 */
snd_status_t snd_inj_hijack_prepare_frame(const SND_THREAD_REGISTERS *live, PVOID entry, ULONG_PTR arg1, ULONG_PTR arg2,
                                          SND_THREAD_REGISTERS *out);

/**
 * @brief Executes the hijack tail on the suspended initial thread.
 *
 * Requires stage `SND_INJ_STAGE_PROTECTIONS_SET`. Reads the live context,
 * paints the entry frame from `remote_entry_point` (falling back to
 * `remote_base`) and the supplied arguments, writes the return thunk onto the
 * target stack if provided, applies the context, and resumes the thread.
 *
 * @param ctx Context after `SND_INJ_STAGE_PROTECTIONS_SET`, with `proc_api`,
 *        `thread_api`, and `remote_thread` valid.
 * @param return_thunk Optional address of a benign function in the target to
 *        return into after the payload exits (e.g. ntdll RtlExitUserThread).
 *        NULL leaves the stack home unset (payload must not return).
 * @param arg1 First argument to the payload (BOF args pointer, else 0).
 * @param arg2 Second argument to the payload (BOF arg_len, else 0).
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, or any required callback/stage
 * handle is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the injection stage is not
 * `SND_INJ_STAGE_PROTECTIONS_SET`.
 * @retval SND_STATUS_ARCH_MISMATCH On architectures without register-arg
 * marshalling.
 * @retval Any error returned by a thread-api context callback, the remote
 * stack write, or the resume.
 */
snd_status_t snd_inj_hijack_execute(snd_inj_ctx_t *ctx, PVOID return_thunk, ULONG_PTR arg1, ULONG_PTR arg2);

SND_END_EXTERN_C

#endif // SND_INJECTION_HIJACK_ENGINE_H