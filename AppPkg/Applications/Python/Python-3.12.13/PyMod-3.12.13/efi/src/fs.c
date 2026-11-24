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

#include <wchar.h>
#include <wctype.h>
#include <errno.h>

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellLib.h>

char*
wgetcwd (char *buf, size_t size)
{
  const wchar_t *Cwd;
  size_t len;
  
  if (size == 0 || buf == NULL) {
    errno = EINVAL;
    return NULL;
  }

  Cwd = ShellGetCurrentDir(NULL);
  if (Cwd == NULL) {
    errno = ENOENT;
    return NULL;
  }
  len = wcslen(Cwd);
  if (size < ((len + 1) * sizeof (wchar_t))) {
    errno = ERANGE;
    return (NULL);
  }

  wmemcpy((wchar_t*)buf, Cwd, len);
  ((wchar_t*)buf)[len] = 0;
  
  return buf;
}

char *
path_to_ascii(char* dst, wchar_t *src, size_t size)
{
   int status = UnicodeStrToAsciiStrS(src, dst, size);
   if(status != RETURN_SUCCESS)
      return NULL;

   return dst;
}
