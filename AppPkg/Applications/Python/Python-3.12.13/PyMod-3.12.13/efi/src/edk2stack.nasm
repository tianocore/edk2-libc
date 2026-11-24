;------------------------------------------------------------------------------
;
; Copyright (c) 2011 - 2026, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   edk2stack.nasm
;
; Abstract:
;
;   UEFI stack switch helpers
;
; Notes:
;
;------------------------------------------------------------------------------

;; --*- nasm -*-
;; These helpers are plain C functions, not EFIAPI, so they inherit the
;; compiler's default calling convention: System V (rdi, rsi) under GCC,
;; Microsoft x64 (rcx, rdx) under MSVC. Without this, an MSVC build reads its
;; arguments out of whatever happens to be in rdi/rsi and edk2_switch_stack
;; sets rsp to a garbage address -- the "VS2022 hangs inside ShellCEntryLib
;; after the stack switch" failure that kept MSVC off the 64 MB stack until
;; 2026-09-08.
;; PY_UEFI_MS_ABI is set from MSFT:*_*_*_NASM_FLAGS in the INFs, so the GCC
;; path keeps its existing register use untouched.
%ifdef PY_UEFI_MS_ABI
  %define ARG1 rcx
  %define ARG2 rdx
%else
  %define ARG1 rdi
  %define ARG2 rsi
%endif

section .text

global edk2_revert_stack

edk2_revert_stack:
        mov rax, [rsp]
        add rsp, 0x200-0x8
        mov rsp, [rsp]
        mov [rsp], rax
        ret
        
global edk2_switch_stack
        
edk2_switch_stack:
        add ARG1, ARG2
        mov rax, 0
        mov [ARG1-0x8], rax
        mov [ARG1-0x10], rsp
        mov rax, [rsp]
        mov [ARG1-0x208], rax
        lea rsp, [ARG1-0x208]
        ret

global edk2_read_rsp

edk2_read_rsp:
        mov rax, rsp
        ret

global edk2_pause

edk2_pause:
        pause
        ret
