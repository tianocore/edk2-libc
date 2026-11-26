/** @file
    UEFI Python support: dummy_unistd.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <errno.h>
#include <stdio.h>

#include <efi/unistd.h>

int execve(const char *filename, char *const argv[],
           char *const envp[])
{
   errno = ENOEXEC;
   return -1;
}   

int execv(const char *path, char *const argv[])
{
   errno = ENOEXEC;
   return -1;
}   

int
uname(struct utsname *buf)
{
   if(buf == NULL) {
      errno = EFAULT;
      return -1;
   }
   
   snprintf(buf->sysname, sizeof(buf->sysname), "uefi");
   snprintf(buf->nodename, sizeof(buf->nodename), "unknown");
   snprintf(buf->release, sizeof(buf->release), "unknown");
   snprintf(buf->version, sizeof(buf->version), "unknown");
   snprintf(buf->machine, sizeof(buf->machine), "X86_64");
   
   return 0;
}


