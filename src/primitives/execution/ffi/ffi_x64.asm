; ===========================================================================
; ffi_x64.asm  -  SindriKit Dynamic FFI Execution Bridge (x64 / MASM)
; ===========================================================================

.CODE

snd_invoke_bridge_x64 PROC FRAME

    ; -----------------------------------------------------------------------
    ; Prologue: save 5 non-volatile registers.
    ;   5 pushes * 8 = 40 bytes.  RSP on entry is 8 mod 16, after 5 pushes:
    ;   8 + 40 = 48, which is 0 mod 16.  Perfect alignment for the upcoming sub.
    ; -----------------------------------------------------------------------
    push    rbx
    .pushreg rbx
    push    r12
    .pushreg r12
    push    r13
    .pushreg r13
    push    r14
    .pushreg r14
    push    r15
    .pushreg r15
    .endprolog

    ; -----------------------------------------------------------------------
    ; Save caller parameters into non-volatiles.
    ; -----------------------------------------------------------------------
    mov     r12, rcx                ; r12 = pFunc
    mov     r13, rdx                ; r13 = pArgs
    mov     r14d, r8d               ; r14 = dwCount (zero-extended)

    ; -----------------------------------------------------------------------
    ; Compute dynamic frame allocation:
    ;   extra_slots = max(0, dwCount - 4)   (QWORD slots for args beyond 4)
    ;   alloc       = ALIGN_UP(32 + extra_slots * 8, 16)
    ; -----------------------------------------------------------------------
    xor     r15, r15                ; r15 = extra_slots = 0
    cmp     r14, 4
    jbe     compute_alloc
    mov     r15, r14
    sub     r15, 4                  ; r15 = dwCount - 4

compute_alloc:
    mov     rax, r15
    shl     rax, 3                  ; rax = extra_slots * 8
    add     rax, 32                 ; add shadow space
    add     rax, 15
    and     rax, 0FFFFFFFFFFFFFFF0h ; round up to 16-byte boundary
    mov     r15, rax                ; r15 = frame_alloc (saved for epilogue)
    sub     rsp, r15                ; commit the frame

    ; -----------------------------------------------------------------------
    ; Spill stack arguments (arg[4] .. arg[dwCount-1]) into [RSP+32+...]
    ; -----------------------------------------------------------------------
    cmp     r14, 4
    jbe     fill_regs               ; nothing to spill

    lea     rax, [rsp + 32]         ; rax -> first stack-arg slot
    mov     rcx, 4                  ; start index

stack_spill_loop:
    cmp     rcx, r14
    jge     fill_regs
    mov     rbx, QWORD PTR [r13 + rcx * 8]
    mov     QWORD PTR [rax], rbx
    add     rax, 8
    inc     rcx
    jmp     stack_spill_loop

    ; -----------------------------------------------------------------------
    ; Assign register arguments RCX, RDX, R8, R9 (with bounds checks).
    ; -----------------------------------------------------------------------
fill_regs:
    xor     rcx, rcx
    xor     rdx, rdx
    xor     r8,  r8
    xor     r9,  r9

    test    r14, r14
    jz      do_call
    mov     rcx, QWORD PTR [r13 + 0]     ; arg[0] -> RCX

    cmp     r14, 2
    jb      do_call
    mov     rdx, QWORD PTR [r13 + 8]     ; arg[1] -> RDX

    cmp     r14, 3
    jb      do_call
    mov     r8,  QWORD PTR [r13 + 16]    ; arg[2] -> R8

    cmp     r14, 4
    jb      do_call
    mov     r9,  QWORD PTR [r13 + 24]    ; arg[3] -> R9

    ; -----------------------------------------------------------------------
    ; Invoke.  RSP is now 16-byte aligned.  Result lands in RAX.
    ; -----------------------------------------------------------------------
do_call:
    call    r12

    ; -----------------------------------------------------------------------
    ; Epilogue.
    ; -----------------------------------------------------------------------
    add     rsp, r15                ; restore RSP

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    ret

snd_invoke_bridge_x64 ENDP

end
