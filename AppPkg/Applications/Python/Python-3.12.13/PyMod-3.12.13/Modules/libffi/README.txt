Vendored libffi (UEFI / ctypes) — Phase 8.5
============================================

Standard reference: edk2-py312 checkout, submodule edk2-libffi @ 1fcd48b
(EFI/LibFFI/LibFFI.inf). Upstream URL for provenance:

  https://github.com/intel-sandbox/firmware.boot.uefi.python38.edk2-libffi

Layout in this tree:

  include/           ffi.h, fficonfig.h, ffitarget.h (edk2-libffi/EFI/LibFFI/Include)
  libffi/include/    ffi_common.h, tramp.h, ffi_cfi.h (edk2-libffi/EFI/LibFFI/libffi/include)
  src/               closures.c, debug.c, prep_cif.c, raw_api.c, types.c
  src/x86/           ffi64.c, ffiw64.c, unix64.S, win64.S, internal64.h, asmnames.h

Linked into monolithic Python312.inf (no LibFFI.dec on PACKAGES_PATH).
Python312.inf: -I include/ + libffi/include/ on CC_FLAGS and X64 PP_FLAGS
(edk2-libffi LibFFI.inf); -DNO_MSABI_VA_FUNCS.

AppPkg-only (not in upstream edk2-libffi tree): `include/edk2_libffi_asm.h` included
from `unix64.S` / `win64.S` — neutralizes `_CET_ENDBR` when monolithic PP uses
`pp_resp.txt` (see Migration Status Phase 8 §8.5).

_ctypes C sources with UEFI deltas live in PyMod-3.12.13/Modules/_ctypes/
(copied from edk2-cpython 3.12.13 UEFI port).

License: libffi license in upstream edk2-libffi repository.
