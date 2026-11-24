/** @file
    UEFI Python support: python.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

/* Minimal main program -- everything is loaded from the library */

#include "Python.h"

#ifdef UEFI_C_SOURCE
#include "efi/py312boot.h"
#include "efi/edk2console_api.h"
#include "internal/pycore_pymem.h"

extern char *(*PyOS_ReadlineFunctionPointer)(FILE *, FILE *, const char *);

void
py312_uefi_reentry_cleanup(void)
{
    if (!Py_IsInitialized()) {
        return;
    }
    py312_boot_print_ascii("py312_uefi_reentry_cleanup enter");
    edk2_console_detach_readline();
    (void)Py_FinalizeEx();
    py312_boot_print_ascii("py312_uefi_reentry_cleanup leave");
}

int
main(int argc, char **argv)
{
    int rc;
    py312_boot_print_ascii("main enter");
    /* Match 3.6.8 UEFI entry: malloc allocator before interpreter bootstrap. */
    (void)_PyMem_SetupAllocators(PYMEM_ALLOCATOR_MALLOC);
    py312_boot_print_ascii("after _PyMem_SetupAllocators");
    *stderr = *stdout;
    PyOS_ReadlineFunctionPointer = NULL;
    edk2_console_detach_readline();
    /* Gated as of 2026-09-09, same reasoning as the "Python312: UefiMain" line
     * in edk2main.c. These two were the only ungated boot output, and together
     * they were the whole console of a GCC image. This one used fputs to stdout
     * rather than firmware Print(), which is why it outlived the first pass at
     * turning the traces off. The fflush goes with it: it existed to push this
     * line out, not for anything downstream. */
#ifdef PY_UEFI_BOOT_TRACE
    fputs("Python312: enter main\n", stdout);
    fflush(stdout);
#endif
    py312_boot_print_ascii("before Py_BytesMain");
    rc = Py_BytesMain(argc, argv);
    py312_boot_print_ascii("after Py_BytesMain");
    return rc;
}
#elif defined(MS_WINDOWS)
int
wmain(int argc, wchar_t **argv)
{
    return Py_Main(argc, argv);
}
#else
int
main(int argc, char **argv)
{
    return Py_BytesMain(argc, argv);
}
#endif
