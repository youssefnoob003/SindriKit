; ===========================================================================
; invoke_indirect_x86.asm  -  SindriKit Indirect Syscall Invocation ASM Stub
; ===========================================================================
;
; Exports:
;   snd_syscall_indirect_invoke_asm
;
; Invokes a syscall indirectly by jumping into the middle of the NTDLL stub.
; ===========================================================================

.686
.model flat, c
.code

PUBLIC snd_syscall_indirect_invoke_asm

; NTSTATUS snd_syscall_indirect_invoke_asm(snd_syscall_args_t* args)
snd_syscall_indirect_invoke_asm PROC
    push ebp
    mov ebp, esp

    ; Save non-volatile registers
    push edi
    push esi
    push ebx

    mov eax, [ebp+8]

    mov ecx, [eax+48]

    test ecx, ecx
    jnz GadgetValid
    mov eax, 0C000000Dh
    jmp Cleanup

GadgetValid:
    ; Push 11 arguments
    push [eax+44] ; arg11
    push [eax+40] ; arg10
    push [eax+36] ; arg9
    push [eax+32] ; arg8
    push [eax+28] ; arg7
    push [eax+24] ; arg6
    push [eax+20] ; arg5
    push [eax+16] ; arg4
    push [eax+12] ; arg3
    push [eax+8]  ; arg2
    push [eax+4]  ; arg1

    ; Extract SSN into EAX for the syscall
    movzx eax, word ptr [eax]

    lea edx, SyscallCleanup
    push edx

    jmp ecx

SyscallCleanup:
    lea esp, [ebp - 12]

Cleanup:
    ; Restore non-volatile registers
    pop ebx
    pop esi
    pop edi

    pop ebp
    ret
snd_syscall_indirect_invoke_asm ENDP

end
