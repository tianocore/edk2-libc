#------------------------------------------------------------------------------
#
# Copyright (c) 2011 - 2023, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Module Name:
#
#   cpu_ia32_gcc.s
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
#    IN   UINT32    rax_value    // rdx
#    IN   UINT32    rbx_value    // r8
#    IN   UINT32    rcx_value    // r9
#    IN   UINT32    rdx_value    // r10
#    IN   UINT32    rsi_value    // r11
#    IN   UINT32    rdi_value    // r12
#    )
#------------------------------------------------------------------------------
_swsmi:
    xor eax, eax
    ret
