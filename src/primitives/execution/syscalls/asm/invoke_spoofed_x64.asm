; ===========================================================================
; invoke_spoofed_x64.asm  -  SindriKit Spoofed Syscall Invocation ASM Stub
; ===========================================================================
;
; Exports:
;   snd_syscall_spoofed_invoke_asm
;
; Invokes a syscall indirectly by jumping to a legitimate `syscall` gadget
; within ntdll.dll, but spoofs the return address pushed onto the stack
; so that call stack telemetry sees the spoofed address instead of the payload
; address Uses MASM SEH directives (.pdata/.xdata) to ensure consistent
; unwinding.
; ===========================================================================

.code

PUBLIC snd_syscall_spoofed_invoke_asm

; snd_syscall_spoofed_invoke_asm(SND_SYSCALL_ARGS* args)
snd_syscall_spoofed_invoke_asm PROC FRAME
    push rbp
    .PUSHREG rbp
    push r12
    .PUSHREG r12
    mov rbp, rsp
    .SETFRAME rbp, 0
    ; Allocate 256 bytes to be safe for larger Fat Frames (up to ~240 bytes).
    sub rsp, 256
    .ALLOCSTACK 256
    .ENDPROLOG

    ; RCX points to SND_SYSCALL_ARGS struct

    mov r11, [rcx+96]       ; sys_addr (gadget)
    test r11, r11
    jz InvalidArgs

    mov r8, [rcx+104]       ; spoof_addr (trampoline)
    test r8, r8
    jz InvalidArgs

    mov r12d, dword ptr [rcx+112]      ; spoof_frame_size (DWORD)
    test r12, r12
    jz InvalidArgs

    ; Build the Top-of-Stack Exposure JMP-Trampoline strictly within our local frame
    ; The syscall gadget expects to return to what is at [rsp].
    ; So we must put the Trampoline (RET) at [rsp+0].
    mov [rsp+0], r8
    lea rax, Cleanup
    mov [rsp+8], rax

    mov rax, [rbp+16]
    mov [rsp + r12 + 8], rax

    mov rax, [rcx+40]       ; arg5
    mov [rsp+40], rax
    mov rax, [rcx+48]       ; arg6
    mov [rsp+48], rax
    mov rax, [rcx+56]       ; arg7
    mov [rsp+56], rax
    mov rax, [rcx+64]       ; arg8
    mov [rsp+64], rax
    mov rax, [rcx+72]       ; arg9
    mov [rsp+72], rax
    mov rax, [rcx+80]       ; arg10
    mov [rsp+80], rax
    mov rax, [rcx+88]       ; arg11
    mov [rsp+88], rax

    ; Extract SSN and arguments 1-4
    movzx eax, word ptr [rcx] ; ssn is a WORD
    mov r10, [rcx+8]        ; arg1
    mov rdx, [rcx+16]       ; arg2
    mov r8,  [rcx+24]       ; arg3
    mov r9,  [rcx+32]       ; arg4

    ; Jump to the NTDLL syscall gadget
    jmp r11

InvalidArgs:
    mov eax, 0C000000Dh     ; STATUS_INVALID_PARAMETER
    jmp Cleanup

Cleanup:
    mov rsp, rbp
    pop r12
    pop rbp
    ret
snd_syscall_spoofed_invoke_asm ENDP

end
