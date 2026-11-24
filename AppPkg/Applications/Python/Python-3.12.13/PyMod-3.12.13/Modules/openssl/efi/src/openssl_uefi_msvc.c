/*
 * UEFI + MSVC: CRT intrinsics and DSO entry points not provided by EDK2.
 * GCC uses portable OpenSSL macros; libcrypto is built without dso/*.c.
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include "internal/dso.h"

#if defined(_MSC_VER) && defined(UEFI_C_SOURCE)

unsigned long __cdecl _lrotl(unsigned long val, int shift)
{
    shift &= 31;
    return (val << shift) | (val >> ((32 - shift) & 31));
}

unsigned long __cdecl _lrotr(unsigned long val, int shift)
{
    shift &= 31;
    return (val >> shift) | (val << ((32 - shift) & 31));
}

unsigned long __cdecl _byteswap_ulong(unsigned long x)
{
    return ((x & 0x000000ffUL) << 24)
         | ((x & 0x0000ff00UL) << 8)
         | ((x & 0x00ff0000UL) >> 8)
         | ((x & 0xff000000UL) >> 24);
}

unsigned __int64 __cdecl _byteswap_uint64(unsigned __int64 x)
{
    return ((unsigned __int64)_byteswap_ulong((unsigned long)x) << 32)
         | (unsigned __int64)_byteswap_ulong((unsigned long)(x >> 32));
}

int __cdecl strerror_s(char *buf, size_t bufsz, int errnum)
{
    const char *msg;

    if (buf == NULL || bufsz == 0) {
        return EINVAL;
    }
    if (errnum == 0) {
        buf[0] = '\0';
        return 0;
    }
    msg = strerror(errnum);
    if (msg == NULL) {
        buf[0] = '\0';
        return EINVAL;
    }
    strncpy(buf, msg, bufsz - 1);
    buf[bufsz - 1] = '\0';
    return 0;
}

DSO *DSO_load(DSO *dso, const char *filename, DSO_METHOD *meth, int flags)
{
    (void)dso;
    (void)filename;
    (void)meth;
    (void)flags;
    return NULL;
}

DSO_FUNC_TYPE DSO_bind_func(DSO *dso, const char *symname)
{
    (void)dso;
    (void)symname;
    return NULL;
}

int DSO_free(DSO *dso)
{
    (void)dso;
    return 1;
}

#endif /* _MSC_VER && UEFI_C_SOURCE */
