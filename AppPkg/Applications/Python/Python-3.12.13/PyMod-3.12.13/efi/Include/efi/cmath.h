/** @file
    UEFI Python support: cmath.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#pragma once

#define isfinite(x) (finite(x))

double hypot(double x, double y);
double round(double x);

double acosh(double x);
double asinh(double x);
double atanh(double x);
double log1p(double x);

#if 0
double expm1(double x);
double copysign(double x, double y);
#endif






