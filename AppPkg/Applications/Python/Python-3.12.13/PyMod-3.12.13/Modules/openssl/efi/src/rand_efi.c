/*
 * OpenSSL RAND pool hooks for UEFI (EFI RNG / urandom fallback).
 *
 * Do not call OPENSSL_ia32_rdseed_bytes from here without the correct
 * rand_rdrand.nasm ABI; use uefi_urandom for firmware entropy.
 */

#include <stddef.h>
#include <sys/types.h>

#include <crypto/rand.h>
#include <efi/random.h>

static size_t
uefi_rand_pool_fill(RAND_POOL *pool, size_t entropy_factor)
{
    size_t bytes_needed;
    unsigned char *buffer;
    ssize_t n;

    bytes_needed = rand_pool_bytes_needed(pool, entropy_factor);
    if (bytes_needed == 0)
        return rand_pool_entropy_available(pool);

    buffer = rand_pool_add_begin(pool, bytes_needed);
    if (buffer == NULL)
        return 0;

    n = uefi_urandom(buffer, (ssize_t)bytes_needed, 0);
    if (n <= 0) {
        rand_pool_add_end(pool, 0, 0);
        return rand_pool_entropy_available(pool);
    }

    rand_pool_add_end(pool, (size_t)n, 8 * (size_t)n);
    return rand_pool_entropy_available(pool);
}

int rand_pool_init(void)
{
    return 1;
}

void rand_pool_cleanup(void)
{
}

void rand_pool_keep_random_devices_open(int keep)
{
    (void)keep;
}

size_t rand_pool_acquire_entropy(RAND_POOL *pool)
{
    return uefi_rand_pool_fill(pool, 1);
}

int rand_pool_add_nonce_data(RAND_POOL *pool)
{
    unsigned char buf[16];

    if (uefi_urandom(buf, sizeof(buf), 0) <= 0)
        return 0;
    return rand_pool_add(pool, buf, sizeof(buf), 0);
}

int rand_pool_add_additional_data(RAND_POOL *pool)
{
    unsigned char buf[8];

    if (uefi_urandom(buf, sizeof(buf), 0) <= 0)
        return 0;
    return rand_pool_add(pool, buf, sizeof(buf), 0);
}
