/** @file
    UEFI Python support: time.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>

#include <efi/time.h>

int clock_getres(clockid_t clk_id, struct timespec *res)
{
   if(res) {
      res->tv_sec = 0;
      res->tv_nsec = 1000;
   }

   return 0;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
   struct timeval mtp = {0};
   
   gettimeofday (&mtp, NULL);

   if(tp) {
      tp->tv_sec = mtp.tv_sec;
      tp->tv_nsec = mtp.tv_usec * 1000;
   }

   return 0;   
}

int clock_settime(clockid_t clk_id, const struct timespec *tp)
{
   return EINVAL;
}

