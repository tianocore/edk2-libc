Vendored pyreadline (UEFI readline module) — Phase 8.2
======================================================

Upstream tree (pinned commit):

  https://github.com/intel-sandbox/firmware.boot.uefi.python38.edk2-pyreadline
  Commit: 1e9facede991aef35011afdb9c0ff0479fdebab9

This is **not** GNU readline C sources. UEFI uses:

  - Built-in C module **edk2console** (Python312.inf / PyMod efi glue)
  - Pure Python **readline.py** + **pyreadline/** from this directory

`create_python_pkg.sh` copies readline.py and pyreadline/ into EFI/lib/python3.12/
so `import readline` works at runtime. No extra INF [Sources] for Phase 8.2.

License: doc/COPYING in upstream repo (copied as COPYING here).

AppPkg delta vs 1e9face: `pyreadline/modes/basemode.py` docstring uses `\\space`
(CPython 3.12 SyntaxWarning for `\space` in docstrings).
