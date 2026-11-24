/** @file
    UEFI Python support: unistd.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#pragma once

#define WNOHANG 1

#define UNAME_SIZE_MAX 20

struct utsname {
   char sysname[UNAME_SIZE_MAX];    /* Operating system name (e.g., "Linux") */
   char nodename[UNAME_SIZE_MAX];   /* Name within "some implementation-defined
                                       network" */
   char release[UNAME_SIZE_MAX];    /* Operating system release (e.g., "2.6.28") */
   char version[UNAME_SIZE_MAX];    /* Operating system version */
   char machine[UNAME_SIZE_MAX];    /* Hardware identifier */
};

int execve(const char *filename, char *const argv[],
           char *const envp[]);
int execv(const char *path, char *const argv[]);
int uname(struct utsname *buf);
