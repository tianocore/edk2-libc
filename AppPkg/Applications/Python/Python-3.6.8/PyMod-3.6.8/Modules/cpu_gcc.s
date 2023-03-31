#------------------------------------------------------------------------------
#
# Copyright (c) 2011 - 2023, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Module Name:
#
#   cpu_gcc.s
#
# Abstract:
#
#   swsmi function
#
# Notes:
#
#------------------------------------------------------------------------------

.global _swsmi

.intel_syntax noprefix
.text

#------------------------------------------------------------------------------
#  void
#  _swsmi (
#    unsigned int    smi_code_data    // rcx
#    IN   UINT64    rax_value    // rdx
#    IN   UINT64    rbx_value    // r8
#    IN   UINT64    rcx_value    // r9
#    IN   UINT64    rdx_value    // rsp + 0x28
#    IN   UINT64    rsi_value    // rsp + 0x30
#    IN   UINT64    rdi_value    // rsp + 0x38
#    )
#------------------------------------------------------------------------------
_swsmi:
    push rbx
    push rsi
    push rdi

    # rsp - 0x18

    # setting up GPR (arguments) to SMI handler call
    # notes:
    #   RAX will get partially overwritten (AX) by _smi_code_data (which is passed in RCX)
    #   RDX will get partially overwritten (DX) by the value of APMC port (= 0x00B2)
    mov rax, rdx # rax_value
    mov ax, cx   # smi_code_data
    mov rdx, r10 # rdx_value
    mov rdx, [rsp + 0x040] # rsp + 0x28 + 0x18

    mov rbx, r8  # rbx_value
    mov rcx, r9  # rcx_value
    mov rsi, [rsp + 0x048] # rsi_value
    mov rdi, [rsp + 0x050] # rdi_value

    # this OUT instruction will write WORD value (smi_code_data) to ports 0xB2 and 0xB3 (SW SMI control and data ports)
    out 0x0B2, ax

    # @TODO: some SM handlers return data/errorcode in GPRs, need to return this to the caller

    pop rdi
    pop rsi
    pop rbx
    ret