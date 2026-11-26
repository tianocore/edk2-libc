Vendored pyreadline (UEFI readline module)
==========================================

Public upstream project:

  https://github.com/pyreadline/pyreadline

UEFI deltas (moderate — not stock Windows-only pyreadline):
  - pyreadline/console/edk2.py backend for edk2console
  - readline.py: PY_UEFI_READLINE opt-in stub when os.name == 'uefi'
  - Small basemode.py docstring escape fix for CPython 3.12

This is **not** GNU readline C sources. UEFI uses:

  - Built-in C module **edk2console** (Python312.inf / PyMod efi glue)
  - Pure Python **readline.py** + **pyreadline/** from this directory

`create_python_pkg.sh` / `.bat` copies readline.py and pyreadline/ into
EFI/lib/python3.12/ so `import readline` works at runtime. No extra INF
[Sources] are required for readline packaging.

License: see COPYING in this directory (BSD-type).
Also see Modules/THIRD_PARTY_NOTICES.txt.

AppPkg delta: `pyreadline/modes/basemode.py` docstring uses `\\space`
(CPython 3.12 SyntaxWarning for `\space` in docstrings).
