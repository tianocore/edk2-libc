/** @file
    UEFI Python support: py312boot.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

/** UEFI boot trace helper (PY_UEFI_BOOT_TRACE). */
#ifndef EFI_PY312BOOT_H
#define EFI_PY312BOOT_H

#ifdef PY_UEFI_BOOT_TRACE
void py312_boot_print_ascii(const char *msg);
#else
static inline void py312_boot_print_ascii(const char *msg)
{
    (void)msg;
}
#endif

/** If Shell re-runs this image without unloading, finish a prior interpreter. */
void py312_uefi_reentry_cleanup(void);

/** After Python exits: release OpenSSL global state (FULL / import ssl). */
void py312_uefi_openssl_disarm(void);

/** VS2022 FULL: clear OpenSSL error queue after finalize (GCC parity). */
void py312_uefi_phase8_after_finalize(void);

/** Call from _ssl module init when OpenSSL is active this run. */
void py312_uefi_note_ssl_used(void);

#endif
