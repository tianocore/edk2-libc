/** @file
    UEFI Python support: edk2console_api.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

/** Minimal API for Python core (no UEFI types). */
#pragma once

void edk2_console_detach_readline(void);

/** Call at UefiMain after console_in is opened (Shell re-launch after REPL). */
void edk2_console_prepare_for_launch(void);

/** Detach readline hooks and restore ConIn/ConOut for Shell (also safe before Py_Finalize). */
void edk2_console_handoff_to_shell(void);

/** ConOut restore only (after edk2_console_detach_readline). */
void edk2_console_restore_for_shell(void);
