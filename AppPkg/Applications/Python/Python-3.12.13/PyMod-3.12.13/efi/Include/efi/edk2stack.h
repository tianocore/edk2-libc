/** @file
    UEFI Python support: edk2stack.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#pragma once

#include <stdint.h>

#define PY_UEFI_DEFAULT_STACK_SIZE (64*1024*1024)

/* Reserve below the usable stack bound so PyOS_CheckStack() trips while there
 * is still room to raise and unwind the MemoryError. */
#ifndef PY_UEFI_STACK_MARGIN
#define PY_UEFI_STACK_MARGIN (8*1024)
#endif

void edk2_switch_stack(uint64_t stack, uint64_t size);
void edk2_revert_stack(void);
uint64_t edk2_read_rsp(void);
