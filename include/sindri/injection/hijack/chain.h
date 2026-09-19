#ifndef SND_INJECTION_HIJACK_CHAIN_H
#define SND_INJECTION_HIJACK_CHAIN_H

#include <sindri/common/macros.h>
#include <sindri/injection/context.h>
#include <sindri/injection/hijack/engine.h>

SND_BEGIN_EXTERN_C

typedef struct _snd_ldr_pe_ctx   snd_ldr_pe_ctx_t;
typedef struct _snd_ldr_coff_ctx snd_ldr_coff_ctx_t;

/**
 * @brief Executes the full thread-hijack injection pipeline for shellcode.
 *
 * Runs: create_suspended_target -> classic alloc -> write -> protect ->
 * hijack execute. The payload starts at `remote_base` (or `remote_entry_point`
 * if pre-set).
 *
 * @param ctx Initialized injection context with target_image_path, payload,
 *        proc_api, and thread_api set.
 * @param return_thunk Optional benign return target (e.g. RtlExitUserThread).
 * @retval SND_OK On success.
 * @retval Any error returned by the shared target/staging/hijack steps.
 */
snd_status_t snd_inj_hijack_shell(snd_inj_ctx_t *ctx, PVOID return_thunk);

/**
 * @brief High-level orchestrator that links a PE loader context and a hijack
 * injection context.
 *
 * The PE is baked locally (parse, arch check, map, relocations against the
 * remote base, imports), marshalled remotely, then executed on the suspended
 * initial thread.
 *
 * @param ldr_ctx Initialized PE loader context with raw_source, mem_api, and
 *        mod_api set.
 * @param inj_ctx Initialized injection context with target_image_path,
 *        proc_api, and thread_api set.
 * @param return_thunk Optional benign return target (e.g. RtlExitUserThread).
 * @retval SND_OK On success.
 * @retval SND_STATUS_ARCH_MISMATCH If the payload architecture is incompatible.
 * @retval Any error returned by the loader, staging, or hijack steps.
 */
snd_status_t snd_inj_hijack_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx, PVOID return_thunk);

/**
 * @brief High-level orchestrator that links a COFF loader context and a hijack
 * injection context.
 *
 * Sections and symbols are prepared locally, marshalled remotely with the BOF
 * argument buffer, then executed on the suspended initial thread.
 *
 * @param ldr_ctx Initialized COFF loader context with raw_source, mem_api, and
 *        mod_api set.
 * @param inj_ctx Initialized injection context with target_image_path,
 *        proc_api, and thread_api set.
 * @param return_thunk Optional benign return target (e.g. RtlExitUserThread).
 * @param entry_point The name of the BOF entry point to execute (e.g., "go").
 * @param args The BOF arguments buffer.
 * @param arg_len The length of the BOF arguments buffer.
 * @retval SND_OK On success.
 * @retval SND_STATUS_ARCH_MISMATCH If the payload architecture is incompatible.
 * @retval Any error returned by the loader, staging, or hijack steps.
 */
snd_status_t snd_inj_hijack_coff(snd_ldr_coff_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx, PVOID return_thunk,
                                 const char *entry_point, void *args, int arg_len);

SND_END_EXTERN_C

#endif // SND_INJECTION_HIJACK_CHAIN_H