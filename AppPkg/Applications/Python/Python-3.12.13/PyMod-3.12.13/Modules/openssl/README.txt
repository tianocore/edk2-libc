Vendored OpenSSL 1.1.1f (UEFI libcrypto / libssl)
=================================================

Public upstream (license + version pin):

  https://github.com/openssl/openssl/tree/OpenSSL_1_1_1f
  Tag: OpenSSL_1_1_1f

UEFI deltas (not an unmodified upstream drop):
  - efi/ packaging glue (opensslconf, rand_efi, MSVC helpers, INF stubs)
  - OPENSSL_SYS_UEFI / UEFI_C_SOURCE conditionals in selected crypto/ssl files
  - Python312.inf compiles a subset of this tree (FULL build only)

UEFI packaging of this 1.1.1f tree follows the historical LibOpenSSL.inf-style
layout used by Python UEFI ports (crypto/, ssl/, engines/, include/, efi/ glue).
Refresh from a matching OpenSSL 1.1.1f UEFI tree and keep Python312.inf
[Sources] in sync. OpenSSL 3.x is a separate port.

Layout in this tree:

  e_os.h             OpenSSL internal OS abstraction (required on -I vendor root)
  crypto/            libcrypto sources (subset compiled — see Python312.inf)
  ssl/               libssl sources (see Python312.inf)
  engines/           engine stubs referenced by LibOpenSSL.inf
  include/           public OpenSSL headers (<openssl/*.h>)
  efi/include/       UEFI opensslconf.h, buildinf.h, dso_conf.h
  efi/src/           rand_efi.c, eng_dyn.c, ui_openssl.c (LibOpenSSL glue)
  LICENSE            OpenSSL 1.1.1f dual OpenSSL + SSLeay license text

Monolithic Python312.inf [BuildOptions] (match LibOpenSSL.inf):

  -I efi/include -I openssl root -I include
  -DNO_MSABI_VA_FUNCS
  -Wno-error=maybe-uninitialized -Wno-error=unused-but-set-variable

Built-in: Modules/_hashopenssl.c registers as **_hashlib** (UEFI_C_SOURCE allows
non-threaded OpenSSL via threads_none.c).

License: see LICENSE in this directory. Also see Modules/THIRD_PARTY_NOTICES.txt.
