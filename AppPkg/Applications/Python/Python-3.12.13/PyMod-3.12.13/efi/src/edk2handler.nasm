; -*- nasm -*-
;------------------------------------------------------------------------------ ;
; Copyright (c) 2011 - 2026, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   edk2handler.nasm
;
; Abstract:
;
;   x64 CPU Exception Handler
;
; Notes:
;
;   This file is a modified version of
;   edk2/UefiCpuPkg/Library/CpuExceptionHandlerLib/X64/ExceptionHandlerAsm.nasm
;
;------------------------------------------------------------------------------


extern ASM_PFX(py_error_code_flag)    ; Error code flags for exceptions
extern ASM_PFX(py_handle_exception)

;; edk2_get_idtr / edk2_set_idtr are plain C functions, not EFIAPI, so their
;; pointer argument arrives in rdi under GCC but rcx under MSVC. See the same
;; block in edk2stack.nasm. PY_UEFI_MS_ABI comes from MSFT:*_*_*_NASM_FLAGS.
;; (py_common_interrupt_entry below is already MS-x64-aware where it calls
;; py_handle_exception, per the EDK2 x64 convention.)
%ifdef PY_UEFI_MS_ABI
  %define ARG1 rcx
%else
  %define ARG1 rdi
%endif

SECTION .data

DEFAULT REL
SECTION .text

ALIGN   8

;---------------------------------------;
; py_common_interrupt_entry                  ;
;---------------------------------------;
; The follow algorithm is used for the common interrupt routine.
; Entry from each interrupt with a push eax and eax=interrupt number
; Stack frame would be as follows as specified in IA32 manuals:
;
; +---------------------+ <-- 16-byte aligned ensured by processor
; +    Old SS           +
; +---------------------+
; +    Old RSP          +
; +---------------------+
; +    RFlags           +
; +---------------------+
; +    CS               +
; +---------------------+
; +    RIP              +
; +---------------------+
; +    Error Code       +
; +---------------------+
; +   Vector Number     +
; +---------------------+
; +    RBP              +
; +---------------------+ <-- RBP, 16-byte aligned
; The follow algorithm is used for the common interrupt routine.
global ASM_PFX(py_common_interrupt_entry)
ASM_PFX(py_common_interrupt_entry):
    cli
    pop     rax
    ;
    ; All interrupt handlers are invoked through interrupt gates, so
    ; IF flag automatically cleared at the entry point
    ;
    xchg    rcx, [rsp]      ; Save rcx into stack and save vector number into rcx
    and     rcx, 0xFF
    cmp     ecx, 32         ; Intel reserved vector for exceptions?
    jae     NoErrorCode
    bt      [ASM_PFX(py_error_code_flag)], ecx
    jc      HasErrorCode

NoErrorCode:

    ;
    ; Push a dummy error code on the stack
    ; to maintain coherent stack map
    ;
    push    qword [rsp]
    mov     qword [rsp + 8], 0
HasErrorCode:
    push    rbp
    mov     rbp, rsp
    push    0             ; clear EXCEPTION_HANDLER_CONTEXT.OldIdtHandler
    push    0             ; clear EXCEPTION_HANDLER_CONTEXT.ExceptionDataFlag

    ;
    ; Stack:
    ; +---------------------+ <-- 16-byte aligned ensured by processor
    ; +    Old SS           +
    ; +---------------------+
    ; +    Old RSP          +
    ; +---------------------+
    ; +    RFlags           +
    ; +---------------------+
    ; +    CS               +
    ; +---------------------+
    ; +    RIP              +
    ; +---------------------+
    ; +    Error Code       +
    ; +---------------------+
    ; + RCX / Vector Number +
    ; +---------------------+
    ; +    RBP              +
    ; +---------------------+ <-- RBP, 16-byte aligned
    ;

    ;
    ; Since here the stack pointer is 16-byte aligned, so
    ; EFI_FX_SAVE_STATE_X64 of EFI_SYSTEM_CONTEXT_x64
    ; is 16-byte aligned
    ;

;; UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
;; UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rax
    push qword [rbp + 8]   ; RCX
    push rdx
    push rbx
    push qword [rbp + 48]  ; RSP
    push qword [rbp]       ; RBP
    push rsi
    push rdi

;; UINT64  Gs, Fs, Es, Ds, Cs, Ss;  insure high 16 bits of each is zero
    movzx   rax, word [rbp + 56]
    push    rax                      ; for ss
    movzx   rax, word [rbp + 32]
    push    rax                      ; for cs
    mov     rax, ds
    push    rax
    mov     rax, es
    push    rax
    mov     rax, fs
    push    rax
    mov     rax, gs
    push    rax

    mov     [rbp + 8], rcx               ; save vector number

;; UINT64  Rip;
    push    qword [rbp + 24]

;; UINT64  Gdtr[2], Idtr[2];
    xor     rax, rax
    push    rax
    push    rax
    sidt    [rsp]
    xchg    rax, [rsp + 2]
    xchg    rax, [rsp]
    xchg    rax, [rsp + 8]

    xor     rax, rax
    push    rax
    push    rax
    sgdt    [rsp]
    xchg    rax, [rsp + 2]
    xchg    rax, [rsp]
    xchg    rax, [rsp + 8]

;; UINT64  Ldtr, Tr;
    xor     rax, rax
    str     ax
    push    rax
    sldt    ax
    push    rax

;; UINT64  RFlags;
    push    qword [rbp + 40]

;; UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
    mov     rax, cr8
    push    rax
    mov     rax, cr4
    or      rax, 0x208
    mov     cr4, rax
    push    rax
    mov     rax, cr3
    push    rax
    mov     rax, cr2
    push    rax
    xor     rax, rax
    push    rax
    mov     rax, cr0
    push    rax

;; UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
    mov     rax, dr7
    push    rax
    mov     rax, dr6
    push    rax
    mov     rax, dr3
    push    rax
    mov     rax, dr2
    push    rax
    mov     rax, dr1
    push    rax
    mov     rax, dr0
    push    rax

;; FX_SAVE_STATE_X64 FxSaveState;
    sub rsp, 512
    mov rdi, rsp
    db 0xf, 0xae, 0x7 ;fxsave [rdi]

;; UEFI calling convention for x64 requires that Direction flag in EFLAGs is clear
    cld

;; UINT32  ExceptionData;
    push    qword [rbp + 16]

;; Prepare parameter and call
    mov     rcx, [rbp + 8]
    mov     rdx, rsp
    ;
    ; Per X64 calling convention, allocate maximum parameter stack space
    ; and make sure RSP is 16-byte aligned
    ;
    sub     rsp, 4 * 8 + 8
    call    ASM_PFX(py_handle_exception)
    add     rsp, 4 * 8 + 8

    cli
;; UINT64  ExceptionData;
    add     rsp, 8

;; FX_SAVE_STATE_X64 FxSaveState;

    mov rsi, rsp
    db 0xf, 0xae, 0xE ; fxrstor [rsi]
    add rsp, 512

;; UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
;; Skip restoration of DRx registers to support in-circuit emualators
;; or debuggers set breakpoint in interrupt/exception context
    add     rsp, 8 * 6

;; UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
    pop     rax
    mov     cr0, rax
    add     rsp, 8   ; not for Cr1
    pop     rax
    mov     cr2, rax
    pop     rax
    mov     cr3, rax
    pop     rax
    mov     cr4, rax
    pop     rax
    mov     cr8, rax

;; UINT64  RFlags;
    pop     qword [rbp + 40]

;; UINT64  Ldtr, Tr;
;; UINT64  Gdtr[2], Idtr[2];
;; Best not let anyone mess with these particular registers...
    add     rsp, 48

;; UINT64  Rip;
    pop     qword [rbp + 24]

;; UINT64  Gs, Fs, Es, Ds, Cs, Ss;
    pop     rax
    ; mov     gs, rax ; not for gs
    pop     rax
    ; mov     fs, rax ; not for fs
    ; (X64 will not use fs and gs, so we do not restore it)
    pop     rax
    mov     es, rax
    pop     rax
    mov     ds, rax
    pop     qword [rbp + 32]  ; for cs
    pop     qword [rbp + 56]  ; for ss

;; UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
;; UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
    pop     rdi
    pop     rsi
    add     rsp, 8               ; not for rbp
    pop     qword [rbp + 48] ; for rsp
    pop     rbx
    pop     rdx
    pop     rcx
    pop     rax
    pop     r8
    pop     r9
    pop     r10
    pop     r11
    pop     r12
    pop     r13
    pop     r14
    pop     r15

    mov     rsp, rbp
    pop     rbp
    add     rsp, 16
    cmp     qword [rsp - 32], 0  ; check EXCEPTION_HANDLER_CONTEXT.OldIdtHandler
    jz      DoReturn
    cmp     qword [rsp - 40], 1  ; check EXCEPTION_HANDLER_CONTEXT.ExceptionDataFlag
    jz      ErrorCode
    jmp     qword [rsp - 32]
ErrorCode:
    sub     rsp, 8
    jmp     qword [rsp - 24]

DoReturn:
    iretq    


global ASM_PFX(edk2_get_idtr)

ASM_PFX(edk2_get_idtr):
        sidt [ARG1+6]
        ret

global ASM_PFX(edk2_set_idtr)

ASM_PFX(edk2_set_idtr):
        pushf
        cli
        lidt [ARG1+6]
        popf
        ret
