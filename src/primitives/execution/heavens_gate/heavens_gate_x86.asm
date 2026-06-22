.386
.model flat, c
.code

; -----------------------------------------------------------------------------
; snd_invoke_hg_x86
;
; Signature:
;   UINT64 snd_invoke_hg_x86(UINT64 pFunctionAddress, const UINT64 *pArgs);
;
; Parameters on 32-bit Stack:
;   [ebp+8]  = pFunctionAddress (Low 32 bits)
;   [ebp+12] = pFunctionAddress (High 32 bits)
;   [ebp+16] = pArgs (Pointer to a strict 6-element UINT64 array)
; -----------------------------------------------------------------------------
snd_invoke_hg_x86 proc
    push ebp
    mov ebp, esp
    push edi
    push esi
    push ebx

    ; 1. Enter Heaven's Gate (CS = 0x33)
    push 33h
    call next_inst
next_inst:
    add dword ptr [esp], 5
    retf

    ; =========================================================================
    ; 64-BIT MODE execution area
    ; =========================================================================

    ; Load pFunctionAddress into RAX
    ; mov rax, qword ptr [rbp+8]
    db 048h, 08Bh, 045h, 008h

    ; Load the padded pArgs array pointer into EBX (Zero-extends to RBX)
    ; mov ebx, dword ptr [rbp+16]
    db 08Bh, 05Dh, 010h

    ; Save the original 64-bit RSP before we misalign it
    ; mov rsi, rsp
    db 048h, 089h, 0E6h

    ; Align stack to a 16-byte boundary (ABI requirement)
    ; and rsp, 0xFFFFFFFFFFFFFFF0
    db 048h, 083h, 0E4h, 0F0h

    ; Allocate 48 bytes of stack space:
    ; (32 bytes for mandatory shadow space + 16 bytes to hold Arg 5 and Arg 6)
    ; sub rsp, 48
    db 048h, 083h, 0ECh, 030h

    ; --- Load Arguments 1 to 4 into Registers (ABI requirement) ---

    ; mov rcx, qword ptr [rbx]      (Arg 1)
    db 048h, 08Bh, 00Bh

    ; mov rdx, qword ptr [rbx+8]    (Arg 2)
    db 048h, 08Bh, 053h, 008h

    ; mov r8, qword ptr [rbx+16]    (Arg 3)
    db 04Ch, 08Bh, 043h, 010h

    ; mov r9, qword ptr [rbx+24]    (Arg 4)
    db 04Ch, 08Bh, 04Bh, 018h

    ; --- Push Arguments 5 and 6 onto the Stack ---

    ; mov r10, qword ptr [rbx+32]   (Load Arg 5)
    db 04Ch, 08Bh, 053h, 020h
    ; mov qword ptr [rsp+32], r10   (Store Arg 5 directly above shadow space)
    db 04Ch, 089h, 054h, 024h, 020h

    ; mov r11, qword ptr [rbx+40]   (Load Arg 6)
    db 04Ch, 08Bh, 05Bh, 028h
    ; mov qword ptr [rsp+40], r11   (Store Arg 6)
    db 04Ch, 089h, 05Ch, 024h, 028h

    ; --- Execute ---

    ; call rax
    db 0FFh, 0D0h

    ; --- Cleanup ---

    ; Restore original RSP
    ; mov rsp, rsi
    db 048h, 089h, 0F4h

    ; 32-bit cdecl expects a 64-bit return value in EDX:EAX.
    ; RAX currently holds the entire 64-bit value. EAX holds the lower 32.
    ; Move upper 32 bits of RAX into EDX.
    ; mov rdx, rax
    db 048h, 089h, 0C2h
    ; shr rdx, 32
    db 048h, 0C1h, 0EAh, 020h

    ; 2. Exit Heaven's Gate (CS = 0x23)
    ; call $+5
    db 0E8h, 000h, 000h, 000h, 000h
    ; mov dword ptr [rsp+4], 23h
    db 0C7h, 044h, 024h, 004h, 023h, 000h, 000h, 000h
    ; add dword ptr [rsp], 0Dh
    db 083h, 004h, 024h, 00Dh
    ; retf
    db 0CBh

    ; =========================================================================
    ; 32-BIT MODE resumed
    ; =========================================================================

    pop ebx
    pop esi
    pop edi
    mov esp, ebp
    pop ebp
    ret
snd_invoke_hg_x86 endp

end
