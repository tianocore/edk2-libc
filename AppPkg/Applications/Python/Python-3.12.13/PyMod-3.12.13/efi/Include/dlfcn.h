/** @file
    UEFI Python support: dlfcn.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#pragma once

#define RTLD_NOW 0

typedef struct {
   const char *dli_fname;  /* Pathname of shared object that
                              contains address */
   void       *dli_fbase;  /* Base address at which shared
                              object is loaded */
   const char *dli_sname;  /* Name of symbol whose definition
                              overlaps addr */
   void       *dli_saddr;  /* Exact address of symbol named
                              in dli_sname */
} Dl_info;

void* dlsym(void *handle, const char *symbol);
char *dlerror(void);
void* dlopen(const char *filename, int flags);
int dlclose(void *handle);
int dladdr(void *addr, Dl_info *info);
