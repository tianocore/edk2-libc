                                EDK II Python
                                   ReadMe
                                Version 3.12.13
                                 AppPkg port
                               FULL (Phase 8, GCC)
                               17 July 2026


1. OVERVIEW
===========
This document describes building, packaging, and running the CPython 3.12.13
interpreter as a UEFI Shell application using the edk2-libc AppPkg layout
(same pattern as Python 3.6.8 in AppPkg/Applications/Python/Python-3.6.8).

Prerequisites:
  * EDK II (tianocore/edk2) with BaseTools built on Linux or WSL.
  * This edk2-libc tree with the Python 3.12.13 AppPkg port (or equivalent).
  * GCC toolchain, NASM, and host Python 3.x for srcprep / frozen steps.

Build guides:
  GCCCompilationBKMs.md                      (GCC / WSL BKMs; companion to 3.6.8)
  Python312_Windows_VS2022_Build_Guide.md    (VS2022 on Windows)

Source alignment (important):
  Sync UEFI CPython deltas from a matching 3.12.13 UEFI CPython tree.
  Do not bulk-sync from a 3.12.0 tree.


2. RELEASE NOTES (FULL — AppPkg Python 3.12.13)
==============================================
  1)  All C extension modules are statically linked into Python312.efi.
  2)  Dynamically loadable extensions (.so / .pyd) are not supported.
  3)  Pure Python stdlib lives under EFI\StdLib\lib\python3.12\ (3.6.8 layout).
  4)  An empty EFI\StdLib\lib\python3.12\lib-dynload\ directory is required so
      getpath can resolve exec_prefix (no .pyd files are needed).
  5)  PACKAGES_PATH = <edk2>;<edk2-libc> only.  Third-party libs are **vendored**
      under PyMod-3.12.13/Modules/ (zlib, libffi, OpenSSL, pyreadline) — do **not**
      add separate edk2-zlib / edk2-openssl / edk2-libffi / edk2-pyreadline
      packages to PACKAGES_PATH.
  6)  **FULL built-ins (static):** zlib, pyreadline (package + edk2console),
      _ctypes / _ctypes_test, _hashlib (OpenSSL libcrypto), _ssl (OpenSSL libssl).
      OpenSSL on UEFI is **1.1.1f** — ssl.OPENSSL_VERSION_INFO will **not** match
      Windows CPython 3.12.x linked to OpenSSL 3.x.
  7)  User-specific site configuration is limited; see site.py on the target.
  8)  Environment variable support is partial (UEFI environ); PYTHONHOME
      can override prefix if the EFI tree is not on fs0:.
  9)  Packaged interpreter name is EFI\Tools\Python.efi (build artifact remains
      Python312.efi).


3. GETTING AND BUILDING PYTHON
==============================

  3.1 Tree layout
  ---------------
  AppPkg/Applications/Python/Python-3.12.13/
    Python-3.12.13/          Upstream CPython 3.12.13 sources (no in-tree UEFI forks)
    PyMod-3.12.13/           UEFI overlays (.c/.h/.py); EFI entry (efi/); see README.txt
    Python312.inf            Monolithic EDK II module (core + Phase 8 vendored libs)
    srcprep.py               Copies PyMod .h/.py into the CPython tree
    patches/                 StdLib patches (apply locally; see section 3.2)
    create_python_pkg.sh     Stage EFI runtime tree for FAT / hardware
    create_python_pkg.bat    Windows variant of packaging script

  3.2 Required: apply libc patches (do NOT commit StdLib on the branch)
  ---------------------------------------------------------------------
  Four patches under patches/ are required for link and runtime behavior.
  Keep StdLib/ vanilla in git; apply before each fresh checkout / build:

    cd <edk2-libc>
    git apply --check --ignore-whitespace \
      AppPkg/Applications/Python/Python-3.12.13/patches/*.patch
    git apply --ignore-whitespace \
      AppPkg/Applications/Python/Python-3.12.13/patches/*.patch

    ls StdLib/LibC/Uefi/upipe.c

  Files:
    0001-Implement-minimal-emulation-of-pipe-functionality.patch
    0002-Introduce-support-for-ANSI-escape-codes-for-console.patch
    0003-Fix-uninitialized-static-variable.patch
    0004-Fix-ioctl-vararg-handling-for-Console-and-Shell-devi.patch

  To discard local StdLib dirt and re-apply:
    git checkout -- StdLib StdLibPrivateInternalFiles
    git clean -fd StdLib
    (then git apply again)

  3.3 Prep (headers, overlays, frozen)
  ------------------------------------
    cd AppPkg/Applications/Python/Python-3.12.13
    python3 srcprep.py

  Run srcprep after changing PyMod-3.12.13/*.h or overlay *.py.

  Do not add UEFI_C_SOURCE edits under Python-3.12.13/ for paths listed in
  PyMod-3.12.13/README.txt; Python312.inf builds those .c files from PyMod.

  Frozen / deepfreeze for this port live under PyMod-3.12.13/Python/ (frozen_modules/*.h,
  frozen.c, deepfreeze/deepfreeze.c).  They are committed in git — fresh clone:
  srcprep.py then build (no make frozen).  Regen only when changing frozen .py
  inputs: Tools/build/regen_frozen_windows.cmd (Windows host Python 3.12.x) —
  see Python312_Windows_VS2022_Build_Guide.md §6 and GCCCompilationBKMs.md §3.

  3.4 Built-in module set (config.c)
  -----------------------------------
  Enabled extensions are listed in PyMod-3.12.13/Modules/config.c
  (_PyImport_Inittab).  Python312.inf lists all linked .c / asm sources.

  FULL vendored (-D BUILD_PYTHON312_FULL=TRUE):
    zlib, readline (pyreadline in EFI StdLib lib tree), _ctypes, _ctypes_test,
    _hashlib, _ssl

  Vendor trees: PyMod-3.12.13/Modules/zlib/, libffi/, openssl/, readline/

  3.5 Environment variables (build host)
  --------------------------------------
    export PACKAGES_PATH=<path_to_edk2>:<path_to_edk2-libc>
    export EDK2_LIBC_PATH=<path_to_edk2-libc>
    export WORKSPACE=<path_to_edk2>
    export PYTHON_COMMAND=python3

    cd $WORKSPACE
    . edksetup.sh

  Example WORKSPACE layout used in development:
    WORKSPACE=~/src/edk2-py312/edk2
    EDK2_LIBC_PATH=~/src/edk2-libc
    PACKAGES_PATH=$WORKSPACE:$EDK2_LIBC_PATH

  3.6 Build command
  -----------------
  MIN (default):
    build -a X64 -b NOOPT -t GCC \
      -p $EDK2_LIBC_PATH/AppPkg/AppPkg.dsc \
      -D BUILD_PYTHON312

  FULL:
    build -a X64 -b NOOPT -t GCC \
      -p $EDK2_LIBC_PATH/AppPkg/AppPkg.dsc \
      -D BUILD_PYTHON312 -D BUILD_PYTHON312_FULL=TRUE

  Output (typical):
    Build/AppPkg/NOOPT_GCC/X64/Python312.efi
    or under .../AppPkg/Applications/Python/Python-3.12.13/Python312/DEBUG/

  After changing Include/patchlevel.h or core sources, delete the Python312
  build output directory under Build/AppPkg/.../Python-3.12.13 before
  rebuilding so the EFI embeds the correct version string.


4. PYTHON-RELATED PATHS AND FILES (TARGET)
==========================================
PREFIX (from PyMod Include/pyconfig.h; 3.6.8-style):

  PLATFORM     uefi
  PREFIX       fs0:\EFI\StdLib
  EXEC_PREFIX  fs0:\EFI\StdLib
  stdlib       fs0:\EFI\StdLib\lib\python3.12
  platstdlib   fs0:\EFI\StdLib\lib\python3.12\lib-dynload   (empty directory)

Layout produced by create_python_pkg.sh / .bat:

  \EFI
    \Tools
        Python.efi             Packaged name (build artifact is Python312.efi)
    \StdLib
        \lib
            \python3.12        Pure Python from Python-3.12.13/Lib (+ PyMod Lib)
                \lib-dynload   Empty; required for getpath
        \etc                   hosts, resolv.conf, etc. (from StdLib/Efi/StdLib/etc)

If the package is installed on another filesystem (e.g. fs1:), set PYTHONHOME
to that volume's \EFI\StdLib before starting Python.efi.


5. INSTALLING / PACKAGING
=========================
From AppPkg/Applications/Python/Python-3.12.13 (with WORKSPACE and
EDK2_LIBC_PATH set):

  ./create_python_pkg.sh GCC NOOPT X64 /path/to/out

Windows:

  create_python_pkg.bat VS2022 NOOPT X64 C:\path\to\out

Stages Python.efi under EFI\Tools\, Lib/ under EFI\StdLib\lib\python3.12\,
and etc/ under EFI\StdLib\etc\.

Copy the EFI\ tree to a FAT32 UEFI volume.  Map the volume in Shell (e.g. fs0:).


6. EXAMPLE: SOCKET SUPPORT (enabled)
====================================
Python312.inf includes socketmodule.c and links BsdSocketLib / EfiSocketLib.
_socket is registered in config.c.

To disable sockets: remove or comment the socket entries in config.c and
Python312.inf, drop BsdSocketLib from [LibraryClasses], and rebuild.

_ssl uses the same vendored OpenSSL tree as _hashlib; no separate
edk2-openssl on PACKAGES_PATH.


7. RUNNING PYTHON
=================
Run from a FAT partition under the UEFI Shell.

  Shell> fs0:
  FS0:\> cd EFI\Tools
  FS0:\EFI\Tools> Python.efi

Expected banner includes 3.12.13 and platform uefi.

REPL smoke (FULL — after create_python_pkg):

  >>> import sys
  >>> print(sys.version)
  >>> print(sys.platform)
  >>> import os, json
  >>> import zlib; import readline; import ctypes; import hashlib; import ssl
  >>> hashlib.sha256(b"x").hexdigest()
  >>> ssl.create_default_context()

Interactive input: pyreadline (staged under EFI\StdLib\lib\python3.12\) plus
Parser/myreadline.c and edk2console (not GNU readline.so).


8. SUPPORTED C MODULES (FULL)
=============================
Built-in table: PyMod-3.12.13/Modules/config.c

Core / always present:
  atexit, faulthandler, uefi, _signal, _tracemalloc, _codecs, _collections,
  errno, _io, itertools, _sre, _thread, time, _weakref, _abc, _functools,
  _locale, _operator, _stat, _symtable, pwd, marshal, _imp, _ast, _tokenize,
  gc, _warnings, _string

UEFI_C_SOURCE extensions (static, in Python312.efi):
  _struct, array, _contextvars, math, cmath, _elementtree, _datetime, _random,
  _bisect, _heapq, _lsprof, unicodedata, _opcode, _asyncio, _queue, select,
  _xxsubinterpreters, _xxinterpchannels, audioop, _csv, _socket,
  _md5, _sha1, _blake2, _sha3, binascii, _multibytecodec, _decimal,
  xxlimited, _posixsubprocess, edk2console, pyexpat, _typing, _json,
  _multiprocessing, _zoneinfo, _pickle, _statistics, _sha2, mmap, termios,
  zlib, _ctypes, _ctypes_test, _hashlib, _ssl

Pure Python readline: import readline (pyreadline copied by create_python_pkg.*)

Interpreter / frozen (not separate import names):
  Deep-frozen stdlib fragments, getpath, ceval, etc. are inside the EFI image.


9. PURE PYTHON STDLIB
=====================
The create_python_pkg scripts copy AppPkg/.../Python-3.12.13/Lib (and PyMod
Lib overlays when present) to EFI\StdLib\lib\python3.12\.

Not every CPython 3.12.13 Lib file is validated on UEFI.  Start with modules
you need and extend testing over time.

Suggested smoke imports:
  encodings, json, hashlib, ssl, re, os, pathlib, collections, datetime,
  zlib, ctypes, readline


10. TROUBLESHOOTING
===================
  * Link errors mentioning upipe / pipe: libc patch 0001 not applied.
  * "Could not find platform dependent libraries <exec_prefix>": create
    EFI\StdLib\lib\python3.12\lib-dynload on the target volume.
  * Banner shows wrong 3.12.x: wrong sync source or stale Build/ objects;
    rebuild Python312 after wiping its build output.
  * Mixed API / link errors after partial file copies: re-sync whole
    Objects/, Python/, Parser/, Include/, Modules/ from a matching 3.12.13 tree.

# # #
