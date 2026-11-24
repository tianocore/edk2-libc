Vendored OpenSSL 1.1.1f (UEFI libcrypto) — Phase 8.3
=====================================================

Standard reference: edk2-py312 submodule edk2-openssl @ 59db29b27df724b3af1fb5cbe49a3f9b9900e862
(efi/LibOpenSSL/LibOpenSSL.inf). Upstream URL for provenance:

  https://github.com/intel-sandbox/firmware.boot.uefi.python38.edk2-openssl

Layout in this tree (mirrors edk2-openssl repo root):

  e_os.h             OpenSSL internal OS abstraction (required on -I vendor root)
  crypto/            libcrypto sources (subset compiled — see Python312.inf)
  ssl/               libssl sources (Phase 8.4 — see Python312.inf)
  engines/           engine stubs referenced by LibOpenSSL.inf
  include/           public OpenSSL headers (<openssl/*.h>)
  efi/include/       UEFI opensslconf.h, buildinf.h, dso_conf.h
  efi/src/           rand_efi.c, eng_dyn.c, ui_openssl.c (LibOpenSSL glue)

Phase 8.3 links **libcrypto only**; Phase 8.4 adds **libssl** (`ssl/`) and `Modules/_ssl.c`.

Monolithic Python312.inf [BuildOptions] (match LibOpenSSL.inf):

  -I efi/include -I openssl root -I include
  -DNO_MSABI_VA_FUNCS
  -Wno-error=maybe-uninitialized -Wno-error=unused-but-set-variable

Built-in: Modules/_hashopenssl.c registers as **_hashlib** (UEFI_C_SOURCE allows
non-threaded OpenSSL via threads_none.c).

License: OpenSSL license in LICENSE at edk2-openssl root (also under include/).

Refresh (1.1.1 line only): sync crypto/, ssl/, engines/, include/, efi/, and
repo-root e_os.h from a matching UEFI edk2-openssl tree; update the OpenSSL
[Sources] entries in Python312.inf to match that tree's LibOpenSSL.inf.
OpenSSL 3.x is a separate port (new vendor + INF + _ssl/_hashlib glue).
