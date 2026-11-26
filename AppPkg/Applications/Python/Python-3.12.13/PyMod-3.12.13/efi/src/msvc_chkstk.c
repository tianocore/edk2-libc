/** @file
    UEFI Python support: msvc_chkstk.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

/*
 * UEFI + MSVC: stack probe helper referenced by compiler-generated code
 * (e.g. libmpdec transpose.c large stack frames) and libffi win64.asm.
 * FULL builds supply this from libffi_msvc/ffi.c; MIN omits ctypes.
 */

#if defined(_MSC_VER) && defined(UEFI_C_SOURCE)

void __chkstk(void)
{
}

#endif
