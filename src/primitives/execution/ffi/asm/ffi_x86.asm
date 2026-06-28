; ===========================================================================
; ffi_x86.asm  -  SindriKit Dynamic FFI Execution Bridge (x86 / MASM)
; ===========================================================================

.686
.model flat, c

.code

PUBLIC snd_ffi_bridge_x86

snd_ffi_bridge_x86 PROC
    push ebp
    mov ebp, esp

    ; Save non-volatile registers
    push ebx
    push esi
    push edi

    mov eax, [ebp+8]    ; pFunc
    mov ecx, [ebp+12]   ; pArgs
    mov edx, [ebp+16]   ; dwCount

    ; If dwCount == 0, skip pushing
    test edx, edx
    jz do_call

    ; Push arguments in reverse order (right to left)
    lea esi, [ecx + edx * 4 - 4]

push_loop:
    push dword ptr [esi]
    sub esi, 4
    dec edx
    jnz push_loop

do_call:
    call eax

    ; Restore stack pointer (handles both stdcall and cdecl target functions)
    lea esp, [ebp-12]

    pop edi
    pop esi
    pop ebx

    mov esp, ebp
    pop ebp
    ret
snd_ffi_bridge_x86 ENDP

end
