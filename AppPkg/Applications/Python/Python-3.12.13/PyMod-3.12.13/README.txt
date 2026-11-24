PyMod-3.12.13 ? UEFI overlays for Python 3.12.13 (AppPkg)

Source of truth for all UEFI-specific C, headers, and stdlib Python patches.
The stock tree under Python-3.12.13/ should match upstream CPython 3.12.13 for
paths that are forked here; Python312.inf compiles PyMod paths for those .c files.

Before build / packaging:
  python3 srcprep.py
copies PyMod *.h and *.py into the CPython tree (see Py312ReadMe.txt).

EFI application glue lives only under PyMod-3.12.13/efi/ (not Python-3.12.13/efi/).

Phase 8 (FULL): ctypes, ssl, hashlib, zlib, and readline are vendored under
PyMod-3.12.13/Modules/ and enabled in Modules/config.c + Python312.inf.

External / third-party library layout (mirror Python 3.6.8 AppPkg):

  Stock CPython module glue stays under Python-3.12.13/Modules/ when unchanged
  (e.g. Modules/zlibmodule.c). Vendored library C sources and UEFI-adopted module
  trees live under PyMod-3.12.13/Modules/; Python312.inf compiles PyMod paths.

  Phase 8.1 (done):
    Modules/zlib/          edk2-zlib @ 8ae7f507 (INF: PyMod-$(PYTHON_VERSION)/Modules/zlib/*.c)

  Phase 8.2 (done):
    Modules/readline/      edk2-pyreadline @ 1e9face (readline.py + pyreadline/; staged to Lib by create_python_pkg)

  Phase 8.3 (done):
    Modules/openssl/       libcrypto + _hashopenssl.c

  Phase 8.4 (done):
    Modules/openssl/ssl/   libssl + _ssl.c

  Phase 8.5 (done):
    Modules/libffi/        + _ctypes / _ctypes_test

Inventory (mirrored path = same relative path under PyMod-3.12.13/):

  Programs/python.c
  Parser/tokenizer.c
  Objects/complexobject.c, Objects/floatobject.c
  Python/bootstrap_hash.c, fileutils.c, getargs.c, pystate.c, pytime.c,
    sysmodule.c, pylifecycle.c
  Modules/config.c, getpath.c, gcmodule.c, posixmodule.c, timemodule.c,
    signalmodule.c, socketmodule.c, faulthandler.c, mathmodule.c, cmathmodule.c,
    _datetimemodule.c, mmapmodule.c, termios.c, _pickle.c, _posixsubprocess.c,
    main.c, _ssl.c, _hashopenssl.c, binascii.c, pwdmodule.c,
    _io/bufferedio.c, _decimal/_decimal.c, _decimal/docstrings.h, _multiprocessing/multiprocessing.c,
    expat/xmlparse.c
  Python/deepfreeze/deepfreeze.c   (generated; regen -> PyMod only)
  Python/frozen.c                  (INF builds PyMod copy; not overlaid by srcprep)
  Python/frozen_modules/*.h        (committed under PyMod; srcprep does not copy to stock)
  Python/dynamic_annotations.c, mysnprintf.c, pyhash.c
  Modules/clinic/posixmodule.c.h, clinic/_ssl.c.h, clinic/mathmodule.c.h
  Include/pyconfig.h, pymath.h, dlfcn.h, pthread.h
  Include/internal/pycore_bitutils.h
  Modules/_sre/sre_lib.h
  Modules/_decimal/libmpdec/mpdecimal.h
  Modules/_hacl/include/krml/lowstar_endianness.h
  Modules/zlib/              vendored zlib (edk2-zlib 8ae7f507)
  Modules/readline/          vendored pyreadline (edk2-pyreadline 1e9face)
  Modules/openssl/           vendored OpenSSL 1.1.1f (libcrypto + libssl)
  Modules/libffi/            vendored libffi (_ctypes)
  Lib/os.py, pathlib.py, site.py, uefipath.py, socket.py, statistics.py, rlcompleter.py
  Lib/ssl/               UEFI minimal package (__init__.py, _uefi_min.py, _stdlib.py)
  Lib/ctypes/uefi_pythonapi.py
  Lib/importlib/_bootstrap_external.py
  Lib/asyncio/uefi_events.py, Lib/asyncio/__init__.py, Lib/asyncio/selector_events.py
  Lib/ctypes/__init__.py, Lib/ctypes/util.py
  Tools/build/deepfreeze.py, generate_global_objects.py,
    fix_deepfreeze_latin1.py, fix_deepfreeze_statically_allocated.py
  efi/src/py312_openssl_uefi.c
  efi/                     UefiMain, stack/handler NASM, dummies, pyconfig for -I

Do not reintroduce UEFI edits under Python-3.12.13/ for the paths above.

Realign stock tree after dropping in a fresh upstream CPython 3.12.13 tree:
  python Tools/restore_upstream_from_cpython.py --upstream /path/to/cpython-3.12.13 [--apply]
Then run srcprep.py before build (create_python_pkg.* runs srcprep automatically).
