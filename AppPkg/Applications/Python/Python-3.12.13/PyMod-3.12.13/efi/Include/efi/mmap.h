/** @file
    UEFI Python support: mmap.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#pragma once

#define MS_SYNC 4 /* Synchronous memory sync.  */

#define MAP_SHARED  0x01		/* Share changes.  */
#define MAP_PRIVATE 0x02		/* Changes are private.  */

#define PROT_READ   0x1		/* Page can be read.  */
#define PROT_WRITE  0x2		/* Page can be written.  */


int getpagesize(void);    
int munmap (void *__addr, size_t __len);
int msync (void *__addr, size_t __len, int __flags);
void *mmap (void *__addr, size_t __len, int __prot,
            int __flags, int __fd, __off_t __offset);
