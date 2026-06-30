/** @file
  Instantiate errno as declared in <errno.h>.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

int             errno     = 0;
RETURN_STATUS   EFIerrno  = RETURN_SUCCESS;

#if defined(__GNUC__) || defined(__clang__)
#define GLOBAL_USED  __attribute__((used))
#else
#define GLOBAL_USED
#endif

// This is required to keep VC++ happy if you use floating-point
int  GLOBAL_USED  _fltused  = 1;
int  GLOBAL_USED  __sse2_available = 0;   ///< Used by ftol2_sse
