/*
 * IEEE-compatible nanf.c -- public domain.
 */
#include  <LibConfig.h>
#include <sys/EfiCdefs.h>

#include <math.h>
#include <machine/endian.h>

const union __float_u __nanf =
#if BYTE_ORDER == BIG_ENDIAN
  { { 0x7f, 0xc0,     0,    0 } };
#else
  { {    0,    0,  0xc0, 0x7f } };
#endif
