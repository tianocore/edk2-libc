/** @file
    UEFI Python support: dummy_mmap.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <stdlib.h>
#include <stdint.h>

#include <sys/ansi.h>
#include <sys/stat.h>

#include <unistd.h>

#include <efi/mmap.h>

int getpagesize(void)
{
  return 4096;
}

int munmap (void *__addr, size_t __len)
{
  if(__addr != NULL)
    free(__addr);
  return 0;
}
    
int msync (void *__addr, size_t __len, int __flags)
{
  return -1;  
}

void *mmap (void *__addr, size_t __len, int __prot,
            int __flags, int __fd, __off_t __offset)
{
  uint8_t *res = NULL;

  if(__len == 0 && __fd < 0) 
    return NULL;


  if(__len == 0 && __fd >= 0) {
    struct stat st = {0};
    int rc = fstat(__fd, &st);
    if(rc < 0)
      return NULL;

    __len = st.st_size;
    if(__len <= 0)
      return NULL;
  }
  
  res = (uint8_t*)malloc(__len);
  
  if(__fd >= 0) {
    size_t bytes_read = read(__fd, res, __len);
    if(bytes_read < 0 || bytes_read != __len) {
      free(res);
      res = NULL;
    }
  }
  
  return res;
}
