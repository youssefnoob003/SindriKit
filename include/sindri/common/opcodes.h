#ifndef SND_COMMON_OPCODES_H
#define SND_COMMON_OPCODES_H

#include <sindri/internal/windows/types.h>

/**
 * @brief Size in bytes of a 32-bit relative displacement operand (rel32).
 * Used in REL32 relocations, JMP/CALL rel32, and RIP-relative offsets.
 */
#define SND_REL32_DISP_SIZE ((DWORD)sizeof(INT32))

#define SND_OPCODE_JMP_INDIRECT_0 0xFF
#define SND_OPCODE_JMP_INDIRECT_1 0x25
#define SND_OPCODE_RET            0xC3
#define SND_OPCODE_SYSCALL_0      0x0F
#define SND_OPCODE_SYSCALL_1      0x05

#define SND_OPCODE_REX_W_R       0x4C
#define SND_OPCODE_MOV_R64_RM64  0x8B
#define SND_MODRM_R10_RCX        0xD1
#define SND_OPCODE_MOV_EAX_IMM32 0xB8

#endif /* SND_COMMON_OPCODES_H */
