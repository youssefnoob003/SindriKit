; ===========================================================================
; invoke_x86.asm  -  SindriKit Syscall Invocation ASM Stub (x86 / MASM)
; ===========================================================================
;
; Exports:
;   snd_syscall_invoke_asm
;
; Invokes a syscall with up to 11 arguments safely. Handles both native x86
; via int 0x2E and WOW64 execution environments.
; ===========================================================================

.686
.model flat, c

.code

PUBLIC snd_syscall_invoke_asm

snd_syscall_invoke_asm PROC
    ; Setup standard frame
    push ebp
    mov ebp, esp

    ; ebp+8 is SND_SYSCALL_ARGS*
    mov ecx, [ebp+8]

    ; Push 11 arguments from the struct in reverse order
    push [ecx+44] ; arg11
    push [ecx+40] ; arg10
    push [ecx+36] ; arg9
    push [ecx+32] ; arg8
    push [ecx+28] ; arg7
    push [ecx+24] ; arg6
    push [ecx+20] ; arg5
    push [ecx+16] ; arg4
    push [ecx+12] ; arg3
    push [ecx+8]  ; arg2
    push [ecx+4]  ; arg1

    ; SSN
    movzx eax, word ptr [ecx]

    ; Check if running under WOW64
    ASSUME fs:nothing
    mov ecx, fs:[0C0h]
    test ecx, ecx
    jz is_native

    ; WOW64 path expects edx to point to a stack frame containing a return
    ; address followed by the arguments. We push a dummy return address.
    push 0
    mov edx, esp
    call ecx
    add esp, 4
    jmp cleanup

is_native:
    ; Native x86 path expects edx to point directly to the first argument
    mov edx, esp
    int 2Eh

cleanup:
    ; Clean up 44 bytes of pushed args
    add esp, 44

    pop ebp
    ret
snd_syscall_invoke_asm ENDP

end
