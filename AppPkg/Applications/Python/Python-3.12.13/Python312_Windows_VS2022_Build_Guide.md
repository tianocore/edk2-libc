# Build Python 3.12.13 (AppPkg FULL) with VS2022 on Windows

Guide for the **edk2-libc AppPkg** tree on branch **`your-python-3.12.13-branch`**.
Host/workspace setup and FULL build steps for MSVC (**`-t VS2022`**).

**FULL requires `-D BUILD_PYTHON312_FULL=TRUE`** (§7). `AppPkg.dsc` defaults that
switch to **FALSE**, so **`-D BUILD_PYTHON312`** alone builds **MIN**.

**Companion docs (same folder):**

- [`Py312ReadMe.txt`](./Py312ReadMe.txt) — overview, layout, modules
- [`GCCCompilationBKMs.md`](./GCCCompilationBKMs.md) — GCC / WSL BKMs

**PACKAGES_PATH:** `<edk2>;<edk2-libc>` only (semicolon on Windows). Vendored zlib,
OpenSSL, libffi, and readline live under **`PyMod-3.12.13/Modules/`**.

**Active libc clone (this port):** set **`EDK2_LIBC_PATH`** to your local edk2-libc
workspace (example below uses **`edk2-libc`**). EDK **`Build\`** paths include
that folder name (e.g. `...\edk2-libc\...\Python312.efi`).

---

## 0. Layout (Windows)

### Two directories (do not confuse them)

| What | Path | Role |
|------|------|------|
| **IDE / git workspace** | `C:\path\to\edk2-libc` | **Fork** of [tianocore/edk2-libc](https://github.com/tianocore/edk2-libc) — **`your-fork/edk2-libc`**. Python 3.12 AppPkg, `AppPkg.dsc`, VS2022 fixes, docs. **Edit and commit here.** |
| **EDK II `WORKSPACE`** | `C:\path\to\edk2` | **tianocore/edk2** clone. BaseTools, `Conf/`, `Build/` output, **`edksetup.bat`**. **Run `build` from here** after `edksetup`. |

Set **`EDK2_LIBC_PATH`** to your edk2-libc clone (folder name may differ; EDK **`Build\`** paths include that name).

**Sibling layout (example):**

```text
C:\path\to\
  edk2\                    ← WORKSPACE (tianocore/edk2)
  edk2-libc\      ← EDK2_LIBC_PATH (VS2022 port workspace)
```

Do **not** set `PACKAGES_PATH=%CD%\edk2;%CD%` unless your current directory is
`C:\path\to` (parent of both clones). Prefer the explicit paths below.

**Environment (cmd.exe — set every new shell before `build`):**

```cmd
set EDK2_LIBC_PATH=C:\path\to\edk2-libc
set PACKAGES_PATH=C:\path\to\edk2;%EDK2_LIBC_PATH%
set NASM_PREFIX=C:\NASM\
cd /d C:\path\to\edk2
call edksetup.bat
```

Optional persistence:

```cmd
set WORKSPACE=C:\path\to\edk2
set EDK_TOOLS_PATH=%WORKSPACE%\BaseTools
set NASM_PREFIX=C:\NASM\
```

(`NASM` need not be on `PATH` if `NASM_PREFIX` ends with `\` and contains `nasm.exe`. Example: `C:\NASM\nasm.exe`.)

On Windows, use a **tianocore `edk2` clone** as **`WORKSPACE`** (same pattern as the 3.6.8 VS2022 workflow).

---

## 1. Install host tools (Phase V1.1)

| Tool | Purpose | Check |
|------|---------|--------|
| **Git for Windows** | clone, `git apply` patches | `git --version` |
| **Python 3.10+** | `srcprep.py`, BaseTools Python | `python --version` |
| **Visual Studio 2022** with **Desktop C++** | `-t VS2022` EDK build | Developer Command Prompt or `edksetup.bat` loads VC |
| **NASM ≥ 2.15** | `edk2stack.nasm`, `edk2handler.nasm`, `rand_rdrand.nasm` | `nasm -v` on PATH or `NASM_PREFIX` |

**NASM (≥ 2.15):** EDK reads **`NASM_PREFIX`** (trailing backslash required).

**This developer setup:**

```cmd
set NASM_PREFIX=C:\NASM\
C:\NASM\nasm.exe -v
```

Alternative: [nasm.us](https://www.nasm.us/) or `choco install nasm` (often
`C:\Program Files\NASM\`). CI uses Chocolatey; local path may differ.

**VS2022:** Install *Desktop development with C++* workload. EDK `edksetup.bat` selects
an installed VS and extends `PATH` (see status log after first `edksetup`).

---

## 2. Clone and prepare edk2 (Phase V1.6)

```cmd
cd C:\path\to
git clone https://github.com/tianocore/edk2.git
cd edk2
git submodule update --init
```

**BaseTools (required before `build`):**

Modern edk2 uses Python to build native BaseTools binaries:

```cmd
cd C:\path\to\edk2
python -m pip install --upgrade pip
python -m pip install -r pip-requirements.txt
python BaseTools\Edk2ToolsBuild.py -t VS2022
call edksetup.bat
```

Verify:

```cmd
if exist BaseTools\Bin\Win32\GenFw.exe (echo BaseTools OK) else (echo BaseTools MISSING)
```

**Do not** test for **`BaseTools\Bin\Win32\build.exe`** — it never exists. **`Edk2ToolsBuild.py`**
produces the native C tools (**`GenFw`**, **`GenFv`**, **`GenSec`**, **`VfrCompile`**, … — 14 **`.exe`**),
while **`build`** is the Python wrapper **`BaseTools\BinWrappers\WindowsLike\build.bat`** put on
**`PATH`** by **`edksetup.bat`**. The end-to-end check is **`build --help`** after **`edksetup`**.

If `Edk2ToolsBuild.py` fails with `No module named 'edk2toolext'`, install edk2
Python requirements from **`pip-requirements.txt`** in the edk2 root.

**Optional smoke** (after BaseTools green): build **Python 3.6.8** with **`-D BUILD_PYTHON368`**
before attempting `BUILD_PYTHON312`. Full steps:
[`../Python-3.6.8/Python368_Windows_VS2022_Build_Guide.md`](../Python-3.6.8/Python368_Windows_VS2022_Build_Guide.md).

---

## 3. edk2-libc branch (Phase V1.2)

```cmd
cd C:\path\to\edk2-libc
git checkout your-python-3.12.13-branch
```

Confirm:

```cmd
dir AppPkg\Applications\Python\Python-3.12.13\Python312.inf
dir AppPkg\Applications\Python\Python-3.12.13\patches\*.patch
```

---

## 4. Apply libc patches (**required**, local only — Phase V1.3)

Same four patches as GCC. **Do not commit** `StdLib/` changes (target policy for upstream edk2-libc).

**Branch note:** If patched StdLib files are **already in the working tree**, **`git apply`**
may fail with *already exists* / *patch does not apply* — skip apply and verify
**`upipe.c`** / **`daAnsi.c`**. Before **final** upstream push, revert StdLib to baseline
and keep **`patches/*.patch`** only.

### Pre-flight — **run this first**, it decides whether to apply at all

```cmd
cd /d %EDK2_LIBC_PATH%
git rev-parse --abbrev-ref HEAD
git status --short StdLib StdLibPrivateInternalFiles
for %P in (AppPkg\Applications\Python\Python-3.12.13\patches\*.patch) do @(git apply --check --ignore-whitespace "%P" >nul 2>&1 && echo NEEDS APPLY     : %~nxP) || (git apply --reverse --check --ignore-whitespace "%P" >nul 2>&1 && echo ALREADY APPLIED : %~nxP) || echo CONFLICT        : %~nxP
```

(Interactive **`cmd`** uses **`%P`**; inside a **`.bat`** file write **`%%P`**.)

| Line | Meaning | Action |
|------|---------|--------|
| **`NEEDS APPLY`** | Not in the tree and applies cleanly | Apply it (below) |
| **`ALREADY APPLIED`** | Reverse-apply succeeds, so the content is **already present** | **Skip** — re-applying fails or double-applies |
| **`CONFLICT`** | Applies in **neither** direction | Partially applied or drifted — **stop** and inspect before building |

**Why the reverse check:** a failing **`git apply --check`** on its own cannot tell *already applied* from a *real conflict* — both just fail. Reverse-applying succeeds **only** if the change is already in the tree, which is exactly the branch-drift case in the note above.

**Expected on `your-python-3.12.13-branch`:** all four **`ALREADY APPLIED`**, and the two checks below confirm it independently.

```cmd
dir StdLib\LibC\Uefi\upipe.c
dir StdLib\LibC\Uefi\Devices\Console\daAnsi.c
```

**`upipe.c`** (added by **0001**) and **`daAnsi.c`** (added by **0002**) are **new files**, so their presence is a reliable applied-marker. The **`git status`** line above should be **empty** before you apply anything — that is your baseline, and it is what makes **`git checkout -- StdLib StdLibPrivateInternalFiles`** a safe undo.

### Apply (only for patches reported `NEEDS APPLY`)

```cmd
cd /d %EDK2_LIBC_PATH%
git apply --ignore-whitespace AppPkg\Applications\Python\Python-3.12.13\patches\0001-Implement-minimal-emulation-of-pipe-functionality.patch
git apply --ignore-whitespace AppPkg\Applications\Python\Python-3.12.13\patches\0002-Introduce-support-for-ANSI-escape-codes-for-console.patch
git apply --ignore-whitespace AppPkg\Applications\Python\Python-3.12.13\patches\0003-Fix-uninitialized-static-variable.patch
git apply --ignore-whitespace AppPkg\Applications\Python\Python-3.12.13\patches\0004-Fix-ioctl-vararg-handling-for-Console-and-Shell-devi.patch
dir StdLib\LibC\Uefi\upipe.c
```

Re-apply after resetting `StdLib` on a clean checkout.

---

## 5. srcprep (Phase V1.4)

```cmd
cd /d %EDK2_LIBC_PATH%\AppPkg\Applications\Python\Python-3.12.13
python srcprep.py
findstr PLATFORM Include\pyconfig.h
```

Expect `#define PLATFORM "uefi"`.

After any **`pyconfig.h`** edit, re-run **`srcprep.py`** and rebuild. Source of truth is
**`PyMod-3.12.13/Include/pyconfig.h`** (and the EFI copy under **`PyMod-3.12.13/efi/Include/`**).
MSVC X64 uses **`/DUEFI_MSVC_64`** in **`Python312.inf`** (LLP64: **`SIZEOF_LONG`** 4);
GCC uses the LP64 **`#else`** branch (**`SIZEOF_LONG`** 8).

---

## 6. Frozen / deepfreeze (Phase V1.5)

`Python312.inf` / `Python312_MIN.inf` compile frozen code from **PyMod** (port overlay):

| Artifact | Path (under `Python-3.12.13/`) |
|----------|--------------------------------|
| 24× marshal headers | **`PyMod-3.12.13/Python/frozen_modules/*.h`** |
| Frozen module table | **`PyMod-3.12.13/Python/frozen.c`** |
| Deepfreeze | **`PyMod-3.12.13/Python/deepfreeze/deepfreeze.c`** |

Stock **`Python/frozen_modules/`** is not used for builds (README only).

### Fresh clone

On a tree with committed PyMod frozen artifacts: after patches (if needed) and **`srcprep.py`**, run **`build`** — **do not** run freeze/regen first.

### Existing clone

After **`git pull`**: re-run **`srcprep.py`** when overlay headers change; rebuild. Regen frozen outputs **only** when you edit frozen **`.py`** sources or refresh deepfreeze/globals.

If you still have old **`Python/frozen_modules/*.h`** from before the PyMod move, delete those files (keep stock **`README.txt`**) and use **`PyMod-3.12.13/Python/frozen_modules/`** from git.

### Regenerate (when developing frozen inputs)

### Pre-flight — **run this first**, regen is usually *not* needed

Frozen artifacts are **committed**. A normal build must **not** regen (§6 *Fresh clone*). Regen **only** after editing frozen **`.py`** inputs or refreshing deepfreeze/globals.

```cmd
py -3.12 -c "import sys, importlib._bootstrap_external as b; assert sys.version_info[:2]==(3,12), sys.version; print(sys.version); print('magic', b._RAW_MAGIC_NUMBER)"
cd /d %EDK2_LIBC_PATH%\AppPkg\Applications\Python\Python-3.12.13
dir /b PyMod-3.12.13\Python\frozen_modules\*.h | find /c /v ""
if exist PyMod-3.12.13\Python\deepfreeze\deepfreeze.c (echo deepfreeze present) else (echo deepfreeze MISSING)
cd /d %EDK2_LIBC_PATH%
git status --short -- AppPkg/Applications/Python/Python-3.12.13/PyMod-3.12.13/Python
```

| Check | Expect | If it differs |
|-------|--------|---------------|
| Host Python | **3.12.x**, **`magic 168627659`** | **Stop.** A different minor rewrites all 24 headers with a **wrong marshal magic** — the build still links and fails at runtime. Install 3.12.x or set **`HOSTPY`** |
| Header count | **24** | Missing artifacts — **`git pull`** / **`git checkout`** the PyMod tree rather than regenerating |
| **`deepfreeze.c`** | **`present`** | Same as above |
| **`git status`** | **empty** | Commit or stash first, otherwise you cannot tell regen output from your own edits |

The batch file re-checks the host interpreter and exits before touching anything, but running the individual steps by hand **bypasses that guard** — which is the case this pre-flight covers.

**Option C — Windows-native (recommended):**

```cmd
cd /d %EDK2_LIBC_PATH%\AppPkg\Applications\Python\Python-3.12.13
Tools\build\regen_frozen_windows.cmd
```

### Post-regen — confirm what actually changed

```cmd
cd /d %EDK2_LIBC_PATH%
git diff --stat -- AppPkg/Applications/Python/Python-3.12.13/PyMod-3.12.13/Python
```

**No output** means regen reproduced the committed artifacts byte-for-byte — the expected result when inputs did not change, and a good determinism check. **Any diff** should be reviewed before commit; the run must also have printed **`.statically_allocated`** and **`remaining single-char &_Py_ID: 0`**. A diff touching **all 24** headers when you edited only one **`.py`** almost always means the **wrong host Python**.

**Host:** **Python 3.12.x** (e.g. **3.12.10** installer) — same **3.12** minor as this **3.12.13** source; marshal magic must be **168627659** (script checks `sys.version_info[:2] == (3, 12)`). Override interpreter: `set HOSTPY=C:\Path\To\python.exe` then run the batch file.

The script writes **`PyMod-3.12.13/Python/frozen_modules/*.h`** and **`PyMod-3.12.13/Python/deepfreeze/deepfreeze.c`**, then runs **in order:** **`Programs\_freeze_module.py`** → **`PyMod-3.12.13\Tools\build\deepfreeze.py`** → **`fix_deepfreeze_statically_allocated.py`** → **`generate_global_objects.py`** → **`fix_deepfreeze_latin1.py`**, and verifies **`.statically_allocated = 1`** in **`deepfreeze.c`**.

**Important:** **`fix_deepfreeze_latin1.py`** is **required** after **`deepfreeze.py`** / **`generate_global_objects.py`**. Running **`generate_global_objects.py`** alone leaves single-char **`&_Py_ID`** in **`deepfreeze.c`** and breaks VS2022 with **C2039** (`_py_d`, `_py__`, …). Prefer **`Tools\build\regen_frozen_windows.cmd`**, which runs the full order.

**Quick fix if the build already fails with C2039 on `deepfreeze.c`:**

```cmd
cd /d C:\path\to\edk2-libc\AppPkg\Applications\Python\Python-3.12.13
py -3.12 PyMod-3.12.13\Tools\build\fix_deepfreeze_latin1.py
```

Expect **`remaining single-char &_Py_ID: 0`**, then rebuild.

### Optional — WSL / edk2-py312 copy

If you freeze on Linux, copy into **PyMod** paths and run the PyMod fix scripts
(see [`GCCCompilationBKMs.md`](./GCCCompilationBKMs.md) §3). Do **not**
copy only into stock **`Python/frozen_modules/`**.

### Verify

```cmd
cd AppPkg\Applications\Python\Python-3.12.13
if exist PyMod-3.12.13\Python\deepfreeze\deepfreeze.c (echo deepfreeze OK) else (echo deepfreeze MISSING)
dir /b PyMod-3.12.13\Python\frozen_modules\*.h | find /c /v ""
```

Expect **24** `.h` files (typical 3.12 set).

---

## 7. Build Python312 with VS2022 (Phase V3–V4)

**`-D BUILD_PYTHON312_FULL=TRUE` is required for the FULL image this guide describes.**
[`AppPkg.dsc`](../../../AppPkg.dsc) defaults **`BUILD_PYTHON312_FULL`** to **FALSE**, so
**`-D BUILD_PYTHON312`** alone selects **`Python312_MIN.inf`** — no zlib, ctypes/libffi, or
OpenSSL (**`_ssl`**, **`_hashlib`**). The MIN build **succeeds**; the omission only surfaces
as **`ModuleNotFoundError`** on hardware.

```cmd
set EDK2_LIBC_PATH=C:\path\to\edk2-libc
set PACKAGES_PATH=C:\path\to\edk2;%EDK2_LIBC_PATH%
set NASM_PREFIX=C:\NASM\
cd /d C:\path\to\edk2
call edksetup.bat
build -t VS2022 -a X64 -b NOOPT -p AppPkg/AppPkg.dsc -D BUILD_PYTHON312 -D BUILD_PYTHON312_FULL=TRUE
```

**Build flavor:** **`NOOPT`** is the flavor carrying VS2022 FULL runtime sign-off (Phase 8
**`-S -c`** matrix, stdio REPL, **`ssl.create_default_context()`** RNG fix). **`RELEASE`**
also builds; substitute it in the paths below if you use it. Keep the flavor **identical**
in the packaging command — a mismatch stages a stale **`.efi`** from an earlier build.

Artifact (typical):

```text
edk2\Build\AppPkg\NOOPT_VS2022\X64\edk2-libc\AppPkg\Applications\Python\Python-3.12.13\Python312\DEBUG\Python312.efi
```

The module directory is **`Python312\`** for FULL and **`Python312_MIN\`** for MIN — the
quickest way to confirm which image you actually built.

### Packaging (Phase V5)

Use **`create_python_pkg.bat`** from **`edk2-libc`**. The copy under plain **`edk2-libc`** is still a Phase 6 **stub** that only prints `TODO: Phase 6`.

```cmd
set WORKSPACE=C:\path\to\edk2
set EDK2_LIBC_PATH=C:\path\to\edk2-libc
cd /d %EDK2_LIBC_PATH%\AppPkg\Applications\Python\Python-3.12.13
create_python_pkg.bat VS2022 NOOPT X64 C:\path\to\edk2-libc\myUEFIPy312
```

The third-from-last argument is the **build flavor** and must match the **`build -b`** used
above (**`NOOPT`** here, **`RELEASE`** if you built RELEASE).

On success you should see **`Python 3.12 EFI package ready at …\EFI\`** (not the TODO line). Output layout:

```text
myUEFIPy312\EFI\Tools\Python.efi
myUEFIPy312\EFI\StdLib\lib\python3.12\
myUEFIPy312\EFI\StdLib\etc\
```

Copy the **`EFI\`** folder to the FAT volume root (e.g. `fs0:\EFI\`), then from UEFI Shell: `fs0:` → `cd EFI\Tools` → `Python.efi`.

### Deploy after Session 10 (REPL / readline)

| Change type | Redeploy |
|-------------|----------|
| C only (`edk2main`, `edk2console`, `main.c`, `pylifecycle.c`, …) | **`EFI\Tools\Python.efi`** |
| **`Lib/site.py`**, **`readline.py`**, other staged stdlib | Full **`EFI\StdLib\lib\python3.12\`** or re-run **`create_python_pkg.bat`** |

**Manufacturing default (VS2022):** stdio REPL; **`import readline`** is a stub unless
**`PY_UEFI_READLINE=1`**. FULL images also stage **pyreadline** under
**`EFI\StdLib\lib\python3.12\`**.

**Smoke (FULL after packaging):**

```text
>>> import sys; print(sys.version); print(sys.platform)
>>> import zlib, ctypes, hashlib, ssl
>>> hashlib.sha256(b"x").hexdigest()
>>> ssl.create_default_context()
```

---

## 8. Troubleshooting

| Issue | Action |
|-------|--------|
| **`ModuleNotFoundError`** for **`zlib`** / **`ctypes`** / **`ssl`** / **`_hashlib`** on target | MIN image was built. Add **`-D BUILD_PYTHON312_FULL=TRUE`** (§7) and repackage; check the module dir is **`Python312\`**, not **`Python312_MIN\`** |
| Packaging can't find **`Python312.efi`**, or stages an old one | **`create_python_pkg.bat`** flavor differs from **`build -b`** (§7 Packaging) |
| `Cannot find BaseTools Bin Win32` | Run `pip-requirements.txt` + `Edk2ToolsBuild.py -t VS2022` |
| `No module named edk2toolext'` | `pip install -r edk2\pip-requirements.txt` |
| NASM not found | Set `NASM_PREFIX` (e.g. `C:\NASM\`) before `edksetup` / `build` |
| Missing frozen headers / `frozen.c` | **`git pull`**; files are under **`PyMod-3.12.13/Python/frozen_modules/`** (§6). Run **`Tools\build\regen_frozen_windows.cmd`** only when changing frozen inputs |
| `git apply` fails | Often **already patched** — verify **`upipe.c`**. After StdLib reset for upstream, apply patches one-by-one with `--ignore-whitespace` |
| OpenSSL symlink on Windows | Optional for monolithic build; `git restore` path under `PyMod-.../LibOpenSSL/openssl` if needed |
| **`LNK2001`** **`OPENSSL_ia32_rdseed_bytes`** / **`rdrand_bytes`** | FULL link: ensure **`PyMod-.../Modules/openssl/efi/src/rand_rdrand.nasm`** is in **`Python312.inf`** and exports those **`global`** symbols (**win64** **`rcx`/`rdx`**) |
| **`ssl.create_default_context()`** hangs (VS2022) | Rebuild after **`rand_rdrand.nasm`** + **`rand_efi.c`**; smoke: `-S -c "import ssl; ssl.create_default_context(); print('ok')"` then Shell **`exit`** |

---

## 9. Checklist (quick)

```text
[ ] VS2022 C++ workload installed
[ ] NASM on PATH or NASM_PREFIX set
[ ] edk2 cloned; BaseTools built; edksetup.bat OK
[ ] EDK2_LIBC_PATH + PACKAGES_PATH set (cmd)
[ ] Four libc patches applied locally; upipe.c exists
[ ] srcprep.py run; PLATFORM "uefi"
[ ] PyMod deepfreeze + 24× frozen_modules/*.h present (§6 — default on fresh clone; else Tools\build\regen_frozen_windows.cmd)
[ ] build -D BUILD_PYTHON312 -D BUILD_PYTHON312_FULL=TRUE -t VS2022
[ ] create_python_pkg.bat; Shell smoke from EFI\Tools\Python.efi
```
