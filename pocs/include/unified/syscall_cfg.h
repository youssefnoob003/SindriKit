#ifndef UNIFIED_SYSCALL_CFG_H
#define UNIFIED_SYSCALL_CFG_H

#include <sindri/primitives/syscalls.h>
#include <unified/common.h>

snd_status_t load_clean_ntdll(snd_buffer_t *buf, snd_ldr_pe_ctx_t *ctx, PVOID *out_base);
void         apply_syscall_style(const syscall_style_t *cfg);

#endif
