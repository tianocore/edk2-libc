;------------------------------------------------------------------------------
;
; Copyright (c) 2011 - 2026, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   rand_rdrand.nasm
;
; Abstract:
;
;   RDSEED/RDRAND for OpenSSL UEFI RNG
;
; Notes:
;
;------------------------------------------------------------------------------

;; -*- nasm -*-
;; RDSEED/RDRAND for OpenSSL rand_lib.c (OPENSSL_RAND_SEED_RDCPU).
;; win64: RCX=buf, RDX=len, return RAX=len (MSVC/EDK VS2022).
;; elf64: RDI=buf, RSI=len, return RAX=len (GCC UEFI).

section .text

%macro DEF_CPU_RANDOM 2
global %1
%1:
%ifidn __OUTPUT_FORMAT__, win64
        mov     r10, rcx
        mov     r11, rdx
        mov     r8, rdx
        test    r11, r11
        jz      %1_done
%1_loop:
        %2
        jnc     %1_loop
        mov     [r10], al
        inc     r10
        dec     r11
        jnz     %1_loop
        mov     rax, r8
%1_done:
        ret
%else
        mov     r10, rdi
        mov     r11, rsi
        mov     r8, rsi
        test    r11, r11
        jz      %1_done
%1_loop:
        %2
        jnc     %1_loop
        mov     [r10], al
        inc     r10
        dec     r11
        jnz     %1_loop
        mov     rax, r8
%1_done:
        ret
%endif
%endmacro

DEF_CPU_RANDOM OPENSSL_ia32_rdseed_bytes, rdseed rax
DEF_CPU_RANDOM OPENSSL_ia32_rdrand_bytes, rdrand rax
