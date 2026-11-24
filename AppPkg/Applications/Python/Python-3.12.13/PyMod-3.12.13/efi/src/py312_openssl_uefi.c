/** @file
    UEFI OpenSSL RNG / entropy glue.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "efi/py312boot.h"
#include "efi/edk2console_api.h"

#if defined(BUILD_PYTHON312_FULL)
#include <openssl/err.h>
#endif

static int py312_openssl_loaded;

void
py312_uefi_note_ssl_used(void)
{
    py312_openssl_loaded = 1;
}

void
py312_uefi_openssl_disarm(void)
{
    py312_boot_print_ascii("py312_uefi_openssl_disarm enter");
    py312_boot_print_ascii("py312_uefi_openssl_disarm leave");
}

void
py312_uefi_phase8_after_finalize(void)
{
#if defined(BUILD_PYTHON312_FULL)
    if (!py312_openssl_loaded) {
        return;
    }
    py312_boot_print_ascii("py312_uefi_phase8_after_finalize");
    ERR_clear_error();
    edk2_console_handoff_to_shell();
    py312_openssl_loaded = 0;
#endif
}
