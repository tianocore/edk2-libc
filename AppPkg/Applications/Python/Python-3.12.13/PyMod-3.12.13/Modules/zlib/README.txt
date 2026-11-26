Vendored zlib for Python 3.12 AppPkg
====================================

Public upstream (license + version pin):

  https://github.com/madler/zlib/tree/v1.2.11
  Tag / version: zlib 1.2.11

UEFI deltas (light — not a full upstream tarball layout):
  - Flat LibZlib-style source set under PyMod-3.12.13/Modules/zlib/
  - Small UEFI_C_SOURCE include in gzguts.h
  - Same role as Python-3.6.8/Modules/zlib/; 3.12 INF maps to
    PyMod-$(PYTHON_VERSION)/Modules/zlib/*.c (FULL build)

Python312.inf also lists stock Modules/zlibmodule.c; -I this directory for zlib.h.

License: see LICENSE in this directory (also at the top of zlib.h).
Also see Modules/THIRD_PARTY_NOTICES.txt.
