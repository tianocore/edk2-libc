"""Insert .statically_allocated = 1 into deepfreeze unicode .state blocks when missing.

Immortal deep-frozen unicode must set statically_allocated or Py_DEBUG hits
_PyUnicodeCheckConsistency (unicodeobject.c): !_Py_IsImmortal for
SSTATE_NOT_INTERNED non-static strings.

Run after Tools/build/deepfreeze.py (regen_frozen_windows.cmd). Idempotent.
See Python312_VS2022_UEFI_Runtime_Notes.md section 5.
"""
import re
from pathlib import Path

p = Path(__file__).resolve().parents[2] / "Python" / "deepfreeze" / "deepfreeze.c"
text = p.read_text(encoding="utf-8")

pattern = re.compile(
    r"(\n            \.ascii = [01],\n)(        \},)",
    re.MULTILINE,
)

def repl(m: re.Match) -> str:
    block = m.group(0)
    if "statically_allocated" in block:
        return block
    return m.group(1) + "            .statically_allocated = 1,\n" + m.group(2)

new_text, n = pattern.subn(repl, text)
if n:
    p.write_text(new_text, encoding="utf-8")
print(f"done, inserted statically_allocated in {n} unicode .state blocks")
if "statically_allocated" not in new_text:
    raise SystemExit("deepfreeze.c still has no statically_allocated markers")
