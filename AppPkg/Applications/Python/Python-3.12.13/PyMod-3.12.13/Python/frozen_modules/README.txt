Generated frozen-module marshal headers for the UEFI AppPkg port.
Committed under PyMod-3.12.13 so the stock Python-3.12.13 tree stays
upstream-clean. PyMod-3.12.13/Python/frozen.c and Modules/getpath.c
include these files.

Regenerate with Tools/build/regen_frozen_windows.cmd (host Python 3.12.x)
when frozen .py inputs change.
