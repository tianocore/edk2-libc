/** @file
    UEFI Python support: edk2asm.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#pragma once

void edk2_pause(void);

#if defined(_MSC_VER)
/* UEFI/EDK2 MSVC builds do not expose intrin.h (see pycore_atomic.h). */
# define EDK2_PAUSE() edk2_pause()
# define EDK2_RW_BARRIER() do { volatile int _edk2_barrier = 0; (void)_edk2_barrier; } while (0)
#else
# define EDK2_PAUSE() __asm__ __volatile__("pause")
# define EDK2_RW_BARRIER() __asm__ __volatile__("" ::: "memory")
#endif
