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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <math.h>
#include <errno.h>
#include <float.h>

#include <efi/cmath.h>

static const double ln2 = 6.93147180559945286227E-01;
static const double two_pow_p28 = 268435456.0; /* 2**28 */
static const double two_pow_m28 = 3.7252902984619141E-09; /* 2**-28 */

double hypot(double x, double y)
{
    double yx;

    x = fabs(x);
    y = fabs(y);
    if (x < y) {
        double temp = x;
        x = y;
        y = temp;
    }
    if (x == 0.)
        return 0.;
    else {
        yx = y/x;
        return x*sqrt(1.+yx*yx);
    }
}

#if 0
double
copysign(double x, double y)
{
    /* use atan2 to distinguish -0. from 0. */
    if (y > 0. || (y == 0. && atan2(y, -1.) > 0.)) {
        return fabs(x);
    } else {
        return -fabs(x);
    }
}
#endif

double
round(double x)
{
    double absx, y;
    absx = fabs(x);
    y = floor(absx);
    if (absx - y >= 0.5)
        y += 1.0;
    return copysign(y, x);
}

double
acosh(double x)
{
    if (isnan(x)) {
        return x+x;
    }
    if (x < 1.) {                       /* x < 1;  return a signaling NaN */
        errno = EDOM;
        return NAN;
    }
    else if (x >= two_pow_p28) {        /* x > 2**28 */
        if (isinf(x)) {
            return x+x;
        }
        else {
            return log(x) + ln2;          /* acosh(huge)=log(2x) */
        }
    }
    else if (x == 1.) {
        return 0.0;                     /* acosh(1) = 0 */
    }
    else if (x > 2.) {                  /* 2 < x < 2**28 */
        double t = x * x;
        return log(2.0 * x - 1.0 / (x + sqrt(t - 1.0)));
    }
    else {                              /* 1 < x <= 2 */
        double t = x - 1.0;
        return log1p(t + sqrt(2.0 * t + t * t));
    }
}

double
asinh(double x)
{
    double w;
    double absx = fabs(x);

    if (isnan(x) || isinf(x)) {
        return x+x;
    }
    if (absx < two_pow_m28) {           /* |x| < 2**-28 */
        return x;                       /* return x inexact except 0 */
    }
    if (absx > two_pow_p28) {           /* |x| > 2**28 */
        w = log(absx) + ln2;
    }
    else if (absx > 2.0) {              /* 2 < |x| < 2**28 */
        w = log(2.0 * absx + 1.0 / (sqrt(x * x + 1.0) + absx));
    }
    else {                              /* 2**-28 <= |x| < 2= */
        double t = x*x;
        w = log1p(absx + t / (1.0 + sqrt(1.0 + t)));
    }
    return copysign(w, x);
}

double
atanh(double x)
{
    double absx;
    double t;

    if (isnan(x)) {
        return x+x;
    }
    absx = fabs(x);
    if (absx >= 1.) {                   /* |x| >= 1 */
        errno = EDOM;
        return NAN;
    }
    if (absx < two_pow_m28) {           /* |x| < 2**-28 */
        return x;
    }
    if (absx < 0.5) {                   /* |x| < 0.5 */
        t = absx+absx;
        t = 0.5 * log1p(t + t*absx / (1.0 - absx));
    }
    else {                              /* 0.5 <= |x| <= 1.0 */
        t = 0.5 * log1p((absx + absx) / (1.0 - absx));
    }
    return copysign(t, x);
}

#if 0
double
expm1(double x)
{
    /* For abs(x) >= log(2), it's safe to evaluate exp(x) - 1 directly; this
       also works fine for infinities and nans.

       For smaller x, we can use a method due to Kahan that achieves close to
       full accuracy.
    */

    if (fabs(x) < 0.7) {
        double u;
        u = exp(x);
        if (u == 1.0)
            return x;
        else
            return (u - 1.0) * x / log(u);
    }
    else
        return exp(x) - 1.0;
}
#endif

double
log1p(double x)
{
    /* For x small, we use the following approach.  Let y be the nearest float
       to 1+x, then

         1+x = y * (1 - (y-1-x)/y)

       so log(1+x) = log(y) + log(1-(y-1-x)/y).  Since (y-1-x)/y is tiny, the
       second term is well approximated by (y-1-x)/y.  If abs(x) >=
       DBL_EPSILON/2 or the rounding-mode is some form of round-to-nearest
       then y-1-x will be exactly representable, and is computed exactly by
       (y-1)-x.

       If abs(x) < DBL_EPSILON/2 and the rounding mode is not known to be
       round-to-nearest then this method is slightly dangerous: 1+x could be
       rounded up to 1+DBL_EPSILON instead of down to 1, and in that case
       y-1-x will not be exactly representable any more and the result can be
       off by many ulps.  But this is easily fixed: for a floating-point
       number |x| < DBL_EPSILON/2., the closest floating-point number to
       log(1+x) is exactly x.
    */

    double y;
    if (fabs(x) < DBL_EPSILON / 2.) {
        return x;
    }
    else if (-0.5 <= x && x <= 1.) {
        /* WARNING: it's possible that an overeager compiler
           will incorrectly optimize the following two lines
           to the equivalent of "return log(1.+x)". If this
           happens, then results from log1p will be inaccurate
           for small x. */
        y = 1.+x;
        return log(y) - ((y - 1.) - x) / y;
    }
    else {
        /* NaNs and infinities should end up here */
        return log(1.+x);
    }
}

