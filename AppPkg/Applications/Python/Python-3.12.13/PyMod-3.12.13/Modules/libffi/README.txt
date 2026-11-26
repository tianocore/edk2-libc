Vendored libffi (UEFI / ctypes)
===============================

Public upstream project:

  https://github.com/libffi/libffi

UEFI deltas (moderate — not a full libffi release tree):
  - x86_64 sources only (subset)
  - include/edk2_libffi_asm.h (AppPkg) included from unix64.S / win64.S to
    neutralize _CET_ENDBR under monolithic PP_RESP
  - Packaged for Python312.inf FULL (no LibFFI.dec on PACKAGES_PATH), following
    the historical LibFFI.inf-style layout used by Python UEFI ports

Layout in this tree:

  include/           ffi.h, fficonfig.h, ffitarget.h
  libffi/include/    ffi_common.h, tramp.h, ffi_cfi.h
  src/               closures.c, debug.c, prep_cif.c, raw_api.c, types.c
  src/x86/           ffi64.c, ffiw64.c, unix64.S, win64.S, internal64.h, asmnames.h
  LICENSE            MIT-style libffi license text

Python312.inf: -I include/ + libffi/include/ on CC_FLAGS and X64 PP_FLAGS;
-DNO_MSABI_VA_FUNCS.

AppPkg-only: `include/edk2_libffi_asm.h` included from `unix64.S` / `win64.S` —
neutralizes `_CET_ENDBR` when monolithic PP uses `pp_resp.txt`.

_ctypes C sources with UEFI deltas live in PyMod-3.12.13/Modules/_ctypes/.

License: see LICENSE in this directory (also in include/ffi.h).
Also see Modules/THIRD_PARTY_NOTICES.txt.
