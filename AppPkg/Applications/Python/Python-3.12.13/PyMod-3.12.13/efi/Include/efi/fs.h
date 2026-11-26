/** @file
    UEFI Python support: fs.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#pragma once

char *wgetcwd (char *buf, size_t size);
char *path_to_ascii(char* dst, wchar_t *src, size_t size);

