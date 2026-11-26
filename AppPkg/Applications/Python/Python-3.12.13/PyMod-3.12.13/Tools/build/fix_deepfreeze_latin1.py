import re
from pathlib import Path

# After deepfreeze.py / generate_global_objects.py: rewrite single-char &_Py_ID(c)
# to _Py_LATIN1_CHR('c') so deepfreeze.c matches pycore_global_strings.h (no 1-char _Py_ID).
# Always run from regen_frozen_windows.cmd or manually after regen — see
# AppPkg/Applications/Python/Python312_VS2022_UEFI_Runtime_Notes.md section 5.

p = Path(__file__).resolve().parents[2] / "Python" / "deepfreeze" / "deepfreeze.c"
text = p.read_text(encoding="utf-8")
text = re.sub(r"&_Py_LATIN1_CHR\((\'.\')\)", r"_Py_LATIN1_CHR(\1)", text)
text = re.sub(r"&_Py_ID\((.)\)", r"_Py_LATIN1_CHR('\1')", text)
p.write_text(text, encoding="utf-8")
left = re.findall(r"&_Py_ID\(.\)", text)
print("done, remaining single-char &_Py_ID:", len(left))
