#ifndef SND_INJECTION_HIJACK_ENGINE_H
#define SND_INJECTION_HIJACK_ENGINE_H

#include <sindri/common/macros.h>
#include <sindri/injection/common/context.h>
#include <sindri/internal/windows/context.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

/** Number of machine words in the x86 entry frame. */
#define SND_INJ_ENTRY_STACK_WORDS 3
/** Required x64 stack alignment before placing the synthetic return slot. */
#define SND_INJ_X64_STACK_ALIGNMENT 16

/** Registers and remote stack data applied by a hijack operation. */
typedef struct {
    SND_THREAD_REGISTERS registers;
    ULONG_PTR            stack[SND_INJ_ENTRY_STACK_WORDS];
    SIZE_T               stack_size;
} snd_inj_entry_frame_t;

/**
 * @brief Paints a complete remote entry frame from intent.
 *
 * Pure function: no I/O. Places @p entry into `ip`, aligns `sp` from the live
 * context via the arch ABI policy, masks `rflags`, and wires arguments for
 * the target ABI. On x64, arguments are placed in `cx`/`dx`; on x86, the
 * return address and arguments are placed in the returned stack frame.
 *
 * @param live Live context captured from the suspended thread (ip unread;
 *        sp and rflags consumed).
 * @param entry Target instruction pointer (payload entry point, never NULL).
 * @param return_thunk Optional return address. A null value means the payload
 *        must not return and no return slot is written.
 * @param arg1  First argument (BOF args pointer, else 0/NULL).
 * @param arg2  Second argument (BOF arg_len, else 0).
 * @param out   Receives the registers and stack frame to apply remotely.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p live, @p out, or @p entry is NULL.
 * @retval SND_STATUS_INVALID_PARAMETERS_COMBINATION If the captured stack
 * pointer cannot accommodate the x86 entry frame.
 */
snd_status_t snd_inj_hijack_prepare_frame(const SND_THREAD_REGISTERS *live, PVOID entry, PVOID return_thunk,
                                          ULONG_PTR arg1, ULONG_PTR arg2, snd_inj_entry_frame_t *out);

/**
 * @brief Executes the hijack tail on the suspended initial thread.
 *
 * Requires stage `SND_INJ_STAGE_PROTECTIONS_SET`. Reads the live context,
 * resolves a return thunk through the engine when `ctx->return_policy` is
 * `SND_INJ_RETURN_GRACEFUL`, paints the entry frame
 * from `remote_entry_point` (falling back to `remote_base`), writes its stack
 * data remotely, applies the context, and resumes the thread.
 *
 * @param ctx Context after `SND_INJ_STAGE_PROTECTIONS_SET`, with `proc_api`,
 *        `thread_api`, `remote_thread` valid, and `return_policy` set.
 * @param arg1 First argument to the payload (BOF args pointer, else 0).
 * @param arg2 Second argument to the payload (BOF arg_len, else 0).
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p ctx, or any required callback/stage
 * handle is NULL.
 * @retval SND_STATUS_INVALID_STAGE If the injection stage is not
 * `SND_INJ_STAGE_PROTECTIONS_SET`. A failed resume leaves the context at
 * `SND_INJ_STAGE_CONTEXT_APPLIED` and it cannot be retried.
 * @retval SND_STATUS_NULL_POINTER If the frame has content but `write_remote`
 * is not available.
 * @retval SND_STATUS_INVALID_PARAMETERS_COMBINATION If `return_policy` is
 * unrecognized.
 * @retval Any error returned by `snd_ntdll_get_active_export`, a thread API
 * context callback, the remote stack write, or the resume.
 */
snd_status_t snd_inj_hijack_execute(snd_inj_ctx_t *ctx, ULONG_PTR arg1, ULONG_PTR arg2);

SND_END_EXTERN_C

#endif // SND_INJECTION_HIJACK_ENGINE_H
