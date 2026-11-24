Vendored zlib for Python 3.12 AppPkg (Phase 8.1)
==================================================

Upstream tree (pinned commit):

  https://github.com/intel-sandbox/firmware.boot.uefi.python38.edk2-zlib
  Commit: 8ae7f507f4bce349533b2d231feb8bf1e4e69859

zlib 1.2.11 sources from efi/LibZlib/LibZlib.inf in that repo. Lives under
PyMod-3.12.13/Modules/zlib/ (same role as Python-3.6.8/Modules/zlib/ in the
3.6.8 port; 3.12 maps INF sources to PyMod-$(PYTHON_VERSION)/Modules/zlib/*.c).

Python312.inf also lists stock Modules/zlibmodule.c; -I this directory for zlib.h.

License: see zlib.h (zlib license).
