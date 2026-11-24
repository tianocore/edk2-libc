# Python 3.12.13 UEFI — GCC compilation BKMs (AppPkg)

Best-known methods for building **Python312.efi** with **GCC** on Linux or WSL.
**PACKAGES_PATH** = edk2 + edk2-libc only (zlib / OpenSSL / libffi / pyreadline are
vendored under `PyMod-3.12.13/Modules/`).

Companion docs:

- [`Py312ReadMe.txt`](./Py312ReadMe.txt) — overview, layout, modules
- [`Python312_Windows_VS2022_Build_Guide.md`](./Python312_Windows_VS2022_Build_Guide.md) — VS2022 on Windows

Tested with WSL2 Ubuntu 22.04 / 24.04 and native Ubuntu; **X64** only.


## 1. Introduction

This document mirrors the role of `Python-3.6.8/GCCCompilationBKMs.rst` for the
**AppPkg** Python **3.12.13** port (`Python312.inf`, `-D BUILD_PYTHON312`).

Assumptions:

- You can build a simple AppPkg application (e.g. `Hello.inf`) with GCC.
- **edk2-libc** includes `AppPkg/Applications/Python/Python-3.12.13/`.
- UEFI CPython sources are aligned to **3.12.13**.

### 1.1. WSL2 on Windows (optional)

If you build on Windows, use WSL2 Ubuntu:

https://learn.microsoft.com/en-us/windows/wsl/install

Prefer cloning edk2 and edk2-libc under `~/src` inside WSL (faster than
`/mnt/c/...` for large builds). Avoid building against a Windows clone mounted
at `/mnt/c` (CRLF / `autocrlf` noise breaks `git apply` checks).


## 2. EDK II build environment

Follow tianocore GCC guidance when in doubt:

https://github.com/tianocore/tianocore.github.io/wiki/Using-EDK-II-with-Native-GCC

Example paths (adjust):

```text
edk2:        $HOME/src/edk2
edk2-libc:   $HOME/src/edk2-libc
gcc:         /usr/bin/gcc
iasl:        /usr/bin/iasl
```

### 2.1. Ubuntu packages

```bash
sudo apt update
sudo apt install -y \
  build-essential uuid-dev iasl git nasm \
  python3 python3-pip python-is-python3 \
  libx11-dev libxext-dev

nasm -v    # >= 2.15 recommended for Python312 NASM sources
gcc --version
python3 --version
```

On older Ubuntu, if `nasm` is too old, install a newer package (same approach
as `Python-3.6.8/GCCCompilationBKMs.rst` — e.g. nasm 2.15.05 `.deb`).

### 2.2. Clone and build edk2 BaseTools

```bash
mkdir -p ~/src && cd ~/src
git clone https://github.com/tianocore/edk2.git
cd edk2
git submodule update --init
make -C BaseTools
export EDK_TOOLS_PATH=$HOME/src/edk2/BaseTools
. edksetup.sh
```

Set `Conf/target.txt` (example):

```text
TOOL_CHAIN_TAG        = GCC
TARGET_ARCH           = X64
```

Use `GCC` or `GCC5` to match your `Conf/tools_def.txt`.

### 2.3. edk2-libc and PACKAGES_PATH

Clone the edk2-libc tree that contains Python 3.12 AppPkg work:

```bash
cd ~/src
git clone <your-edk2-libc-remote> edk2-libc
cd edk2-libc
git checkout <branch-with-python-3.12.13>
```

Environment (every build shell):

```bash
export PACKAGES_PATH=$HOME/src/edk2:$HOME/src/edk2-libc
export EDK2_LIBC_PATH=$HOME/src/edk2-libc
export WORKSPACE=$HOME/src/edk2
export PYTHON_COMMAND=python3
```

Smoke test:

```bash
cd $WORKSPACE
. edksetup.sh
build -a X64 -b NOOPT -t GCC \
  -p $EDK2_LIBC_PATH/AppPkg/AppPkg.dsc \
  -m $EDK2_LIBC_PATH/AppPkg/Applications/Hello/Hello.inf
```

### 2.4. Apply StdLib patches (required — do not commit on the Python branch)

Before building Python312, apply the four patches under
`AppPkg/Applications/Python/Python-3.12.13/patches/` to **this** edk2-libc
tree (skip any patch that is already applied — verify `upipe.c` / `daAnsi.c`):

```bash
cd $EDK2_LIBC_PATH
git apply --check --ignore-whitespace \
  AppPkg/Applications/Python/Python-3.12.13/patches/*.patch
git apply --ignore-whitespace \
  AppPkg/Applications/Python/Python-3.12.13/patches/*.patch
ls StdLib/LibC/Uefi/upipe.c
ls StdLib/LibC/Uefi/Devices/Console/daAnsi.c
```

Do **not** commit resulting `StdLib/` diffs into the Python contribution branch.
Re-apply after `git checkout -- StdLib` on a clean tree.


## 3. Python 3.12.13 AppPkg prep

```bash
cd $EDK2_LIBC_PATH/AppPkg/Applications/Python/Python-3.12.13
python3 srcprep.py
grep PLATFORM Include/pyconfig.h
```

**Frozen / deepfreeze** (under **PyMod**, required for link):

- `PyMod-3.12.13/Python/deepfreeze/deepfreeze.c`
- `PyMod-3.12.13/Python/frozen_modules/*.h` (24 headers)
- `PyMod-3.12.13/Python/frozen.c`

These are committed in the manufacturing tree. After a fresh clone: `srcprep.py`
then build — no freeze step. Regenerate only when changing frozen `.py` inputs;
prefer Windows host Python **3.12.x** via `Tools/build/regen_frozen_windows.cmd`
(see the Windows guide §6). Do **not** regenerate with a non-3.12 host
interpreter (wrong marshal magic).

After changing `Include/patchlevel.h` or core sources, remove the Python312
build output under `Build/AppPkg/.../Python-3.12.13` and rebuild so the EFI
reports the correct version.


## 4. Build Python312.efi

From `$WORKSPACE` (edk2 root):

**MIN** (default; no Phase 8 vendored libs):

```bash
. edksetup.sh
build -a X64 -b NOOPT -t GCC \
  -p "$EDK2_LIBC_PATH/AppPkg/AppPkg.dsc" \
  -D BUILD_PYTHON312
```

**FULL** (zlib, ctypes, OpenSSL `_hashlib` / `_ssl`, pyreadline):

```bash
build -a X64 -b NOOPT -t GCC \
  -p "$EDK2_LIBC_PATH/AppPkg/AppPkg.dsc" \
  -D BUILD_PYTHON312 -D BUILD_PYTHON312_FULL=TRUE
```

Equivalent module build (FULL):

```bash
build -a X64 -b NOOPT -t GCC \
  -p "$EDK2_LIBC_PATH/AppPkg/AppPkg.dsc" \
  -m "$EDK2_LIBC_PATH/AppPkg/Applications/Python/Python-3.12.13/Python312.inf" \
  -D BUILD_PYTHON312 -D BUILD_PYTHON312_FULL=TRUE
```

Expected artifact:

```text
Build/AppPkg/NOOPT_GCC/X64/Python312.efi
```

(toolchain folder name may be `NOOPT_GCC5` depending on `TOOL_CHAIN_TAG`).

Do **not** add `edk2-libffi`, `edk2-openssl`, `edk2-zlib`, or `edk2-pyreadline`
to `PACKAGES_PATH`.

Save logs:

```bash
build ... -D BUILD_PYTHON312 -D BUILD_PYTHON312_FULL=TRUE 2>&1 | tee ~/python312-apppkg-build.log
```


## 5. Package for UEFI Shell

```bash
cd $EDK2_LIBC_PATH/AppPkg/Applications/Python/Python-3.12.13
./create_python_pkg.sh GCC NOOPT X64 /path/to/out
```

Creates (3.6.8-style layout; matches `PREFIX "\\EFI\\StdLib"`):

```text
/path/to/out/EFI/Tools/Python.efi
/path/to/out/EFI/StdLib/lib/python3.12/
/path/to/out/EFI/StdLib/lib/python3.12/lib-dynload/   (empty)
/path/to/out/EFI/StdLib/etc/
```

Copy `EFI\` to a FAT volume. From the Shell:

```text
fs0:
cd EFI\Tools
Python.efi
```

See `Py312ReadMe.txt` §7.


## 6. Common build failures (quick reference)

| Symptom | Action |
|---------|--------|
| `upipe` link error | Apply patch 0001; verify `upipe.c` exists |
| Missing `pyconfig.h` | Run `srcprep.py` |
| Missing `deepfreeze.c` / frozen headers | Pull PyMod frozen artifacts; or regen on host Python 3.12 |
| `PyInit__ssl` / ctypes unresolved on MIN | Add `-D BUILD_PYTHON312_FULL=TRUE` |
| NASM errors | Upgrade nasm (>= 2.15) |
| Banner shows 3.12.0 | Wrong sync tree or stale `Build/` — wipe + rebuild |
| `Could not find platform dependent libraries` | Ensure empty `EFI\StdLib\lib\python3.12\lib-dynload\` |


## 7. Checklist

```text
[ ] apt packages; nasm >= 2.15; python3
[ ] edk2 BaseTools; edksetup.sh; PYTHON_COMMAND=python3
[ ] PACKAGES_PATH / EDK2_LIBC_PATH / WORKSPACE
[ ] branch with Python312.inf + patches/
[ ] git apply patches/*.patch (upipe.c present) — or already applied
[ ] python3 srcprep.py
[ ] deepfreeze.c + frozen_modules/*.h present under PyMod
[ ] build -D BUILD_PYTHON312 [-D BUILD_PYTHON312_FULL=TRUE] -t GCC
[ ] Python312.efi; create_python_pkg.sh; Shell smoke 3.12.13 from EFI\Tools\Python.efi
```
