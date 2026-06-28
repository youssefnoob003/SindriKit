; ===========================================================================
; invoke_x64.asm  -  SindriKit Syscall Invocation ASM Stub (x64 / MASM)
; ===========================================================================
;
; Exports:
;   snd_syscall_invoke_asm
;
; Invokes a syscall with up to 10 arguments safely, setting up the
; required stack and register states for the x64 calling convention.
; ===========================================================================

.code

PUBLIC snd_syscall_invoke_asm

; snd_syscall_invoke_asm(SND_SYSCALL_ARGS* args)
snd_syscall_invoke_asm PROC
    ; Allocate 96 bytes on stack to safely store args 5-11
    ; (32 bytes shadow space + 56 bytes for 7 args + 8 bytes padding for 16-byte alignment)
    sub rsp, 96

    ; RCX points to SND_SYSCALL_ARGS struct
    ; Setup stack arguments (args 5-11) safely in our new frame
    mov r11, [rcx+40]   ; arg5
    mov [rsp+40], r11
    mov r11, [rcx+48]   ; arg6
    mov [rsp+48], r11
    mov r11, [rcx+56]   ; arg7
    mov [rsp+56], r11
    mov r11, [rcx+64]   ; arg8
    mov [rsp+64], r11
    mov r11, [rcx+72]   ; arg9
    mov [rsp+72], r11
    mov r11, [rcx+80]   ; arg10
    mov [rsp+80], r11
    mov r11, [rcx+88]   ; arg11
    mov [rsp+88], r11

    ; Extract SSN and arguments 1-4
    movzx eax, word ptr [rcx] ; ssn is a WORD
    mov r10, [rcx+8]    ; arg1 (must go to r10 for syscall)
    mov rdx, [rcx+16]   ; arg2
    mov r8,  [rcx+24]   ; arg3
    mov r9,  [rcx+32]   ; arg4

    syscall

    ; Restore stack and return
    add rsp, 96
    ret
snd_syscall_invoke_asm ENDP

end
