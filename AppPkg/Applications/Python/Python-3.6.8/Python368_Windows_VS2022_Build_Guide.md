# Build Python 3.6.8 UEFI (AppPkg) with VS2022 on Windows

Step-by-step instructions to produce **`Python.efi`** using **`-D BUILD_PYTHON368`**
on a Windows host. Use this to validate VS2022 + NASM + AppPkg before porting
**Python 3.12.13** (`BUILD_PYTHON312`).

**Related:**

| Doc | Role |
|-----|------|
| [`Py368ReadMe.txt`](./Py368ReadMe.txt) | Original 3.6.8 UEFI overview (paths, install layout) |
| [`GCCCompilationBKMs.rst`](./GCCCompilationBKMs.rst) | GCC / WSL BKMs for 3.6.8 |
| [`.github/workflows/build-python-uefi-vs2022.yaml`](../../../../.github/workflows/build-python-uefi-vs2022.yaml) | CI reference (layout differs from sibling-clone setup below) |
| [`../Python-3.12.13/Python312_Windows_VS2022_Build_Guide.md`](../Python-3.12.13/Python312_Windows_VS2022_Build_Guide.md) | 3.12 VS2022 build (patches, frozen — **not** required for 3.6.8) |
| [`../Python-3.12.13/GCCCompilationBKMs.md`](../Python-3.12.13/GCCCompilationBKMs.md) | 3.12 GCC / WSL BKMs |

---

## 1. What you build

| Item | Value |
|------|--------|
| DSC define | **`BUILD_PYTHON368`** |
| INF | `AppPkg/Applications/Python/Python-3.6.8/Python368.inf` |
| `BASE_NAME` | **`Python`** → output **`Python.efi`** |
| Typical command | `build -t VS2022 -a X64 -b RELEASE -p …\AppPkg\AppPkg.dsc -D BUILD_PYTHON368` |

**Not required for 3.6.8:** Python 3.12 StdLib `patches/`, frozen/deepfreeze, or Phase 8 vendored OpenSSL in the 3.12 tree.

---

## 2. Directory layout (this developer setup)

Two **sibling** clones under one parent (do not nest libc inside edk2). Use the same
**`EDK2_LIBC_PATH`** as the Python 3.12 VS2022 port (**`edk2-libc-jp-vsfix`**).

```text
C:\Users\njayapra\github\
  edk2\                    ← EDK II WORKSPACE (edksetup.bat, Build\)
  edk2-libc-jp-vsfix\      ← EDK2_LIBC_PATH (fork jpshivakavi/edk2-libc-jp)
```

| Variable | Example value |
|----------|----------------|
| `WORKSPACE` | `C:\Users\njayapra\github\edk2` |
| `EDK2_LIBC_PATH` | `C:\Users\njayapra\github\edk2-libc-jp-vsfix` |
| `PACKAGES_PATH` | `C:\Users\njayapra\github\edk2;C:\Users\njayapra\github\edk2-libc-jp-vsfix` |
| `NASM_PREFIX` | `C:\NASM\` (trailing `\` required; NASM need not be on `PATH`) |

---

## 3. Prerequisites (one-time)

1. **Visual Studio 2022** — *Desktop development with C++* workload.
2. **Git** — clone [tianocore/edk2](https://github.com/tianocore/edk2) and your edk2-libc fork.
3. **Python 3.x** — for `srcprep.py` (`python --version`).
4. **NASM ≥ 2.15** — e.g. `C:\NASM\nasm.exe` (verify: `C:\NASM\nasm.exe -v`).

**edk2 submodules + BaseTools** (once per edk2 clone, or after BaseTools updates):

```cmd
cd /d C:\Users\njayapra\github\edk2
git submodule update --init
python -m pip install -r pip-requirements.txt
python BaseTools\Edk2ToolsBuild.py -t VS2022
```

If `Edk2ToolsBuild.py` fails with `No module named 'edk2toolext'`, run `pip install -r pip-requirements.txt` first.

CI sometimes runs `edksetup.bat ForceRebuild`; **`Edk2ToolsBuild.py -t VS2022`** is the supported path on current tianocore edk2.

---

## 4. Full build (cmd.exe)

Use **cmd.exe** (matches GitHub Actions). Set environment **before** `edksetup` and `build`.

### Step 1 — Environment

```cmd
set EDK2_LIBC_PATH=C:\Users\njayapra\github\edk2-libc-jp-vsfix
set PACKAGES_PATH=C:\Users\njayapra\github\edk2;%EDK2_LIBC_PATH%
set NASM_PREFIX=C:\NASM\
set WORKSPACE=C:\Users\njayapra\github\edk2
```

Adjust paths if your clones live elsewhere; keep **`edk2` first**, **libc second** in `PACKAGES_PATH`.

### Step 2 — srcprep (3.6.8)

Copies PyMod headers into the 3.6.8 tree. Run before the first build, or after PyMod header edits.

```cmd
cd /d %EDK2_LIBC_PATH%\AppPkg\Applications\Python\Python-3.6.8
python srcprep.py
```

### Step 3 — edksetup

```cmd
cd /d %WORKSPACE%
call edksetup.bat
```

You may see a warning about `BaseTools\Bin\Win32` on some trees; **`build`** via BinWrappers still works after `Edk2ToolsBuild.py` succeeds.

Optional defaults in `%WORKSPACE%\Conf\target.txt`:

```text
TOOL_CHAIN_TAG   = VS2022
TARGET_ARCH      = X64
TARGET           = RELEASE
```

### Step 4 — Build

From **`%WORKSPACE%`** (still in the same cmd session):

```cmd
build -t VS2022 -a X64 -b RELEASE -p AppPkg/AppPkg.dsc -D BUILD_PYTHON368
```

**Important:** Use **`-p AppPkg/AppPkg.dsc`** (resolved via `PACKAGES_PATH`), **not**
`-p %EDK2_LIBC_PATH%\AppPkg\AppPkg.dsc`. An absolute path to the DSC under
**edk2-libc-jp-vsfix** (your **`EDK2_LIBC_PATH`** clone) makes current EDK **`build`** exit in **0 seconds** with only
`- Failed -` and no compiler output. `EDK2_LIBC_PATH` is still required for
packaging and for `Python312.inf` `$(EDK2_LIBC_PATH)` — only the **`-p` argument**
should stay workspace-relative.

First **RELEASE** build can take a long time (monolithic interpreter + zlib + MSVC `_ctypes` / libffi).

**Success — primary artifact path:**

```cmd
dir %WORKSPACE%\Build\AppPkg\RELEASE_VS2022\X64\Python.efi
```

If not found:

```cmd
dir /s /b %WORKSPACE%\Build\AppPkg\RELEASE_VS2022\X64\Python.efi
```

`create_python_pkg.bat` (below) expects the flat path under `Build\AppPkg\RELEASE_VS2022\X64\`. If your build only emits a deeper path, copy `Python.efi` there or adjust the packaging script.

---

## 5. Package for UEFI (optional)

Stages **`Python.efi`** and stdlib for a FAT volume (3.6.8 layout).

From **`%WORKSPACE%`**, with `EDK2_LIBC_PATH` still set:

```cmd
cd /d %WORKSPACE%
call %EDK2_LIBC_PATH%\AppPkg\Applications\Python\Python-3.6.8\create_python_pkg.bat VS2022 RELEASE X64 myUEFIPy368
```

Output under **`%WORKSPACE%\myUEFIPy368\`**:

```text
myUEFIPy368\EFI\Tools\Python.efi
myUEFIPy368\EFI\StdLib\lib\python36.8\
myUEFIPy368\EFI\StdLib\etc\
```

Note: 3.6.8 uses **`EFI\Tools\Python.efi`**, not the 3.12 **`EFI\bin\Python312.efi`** layout.

---

## 6. One-shot copy-paste

```cmd
set EDK2_LIBC_PATH=C:\Users\njayapra\github\edk2-libc-jp-vsfix
set PACKAGES_PATH=C:\Users\njayapra\github\edk2;%EDK2_LIBC_PATH%
set NASM_PREFIX=C:\NASM\
set WORKSPACE=C:\Users\njayapra\github\edk2

cd /d %EDK2_LIBC_PATH%\AppPkg\Applications\Python\Python-3.6.8
python srcprep.py

cd /d %WORKSPACE%
call edksetup.bat
build -t VS2022 -a X64 -b RELEASE -p AppPkg/AppPkg.dsc -D BUILD_PYTHON368

dir Build\AppPkg\RELEASE_VS2022\X64\Python.efi
call %EDK2_LIBC_PATH%\AppPkg\Applications\Python\Python-3.6.8\create_python_pkg.bat VS2022 RELEASE X64 myUEFIPy368
```

---

## 7. Troubleshooting

| Symptom | Action |
|---------|--------|
| Instant `- Failed -` (00:00:00), no compile lines | **Do not** pass an absolute `-p` to `AppPkg.dsc`; use **`-p AppPkg/AppPkg.dsc`** with `PACKAGES_PATH` set. Also set **`EDK2_LIBC_PATH`** before `build` (needed for packaging). |
| NASM / `.nasm` / `.asm` errors | Set `NASM_PREFIX=C:\NASM\` **before** `edksetup` and `build` |
| INF or AppPkg not found | Check `PACKAGES_PATH` lists **both** edk2 and `%EDK2_LIBC_PATH%` |
| `create_python_pkg.bat` fails | Requires `Build\AppPkg\RELEASE_VS2022\X64\Python.efi` |
| Link / compile errors in StdLib | 3.6.8 CI does not use 3.12 patches; if you applied 3.12 libc patches locally, they should still be compatible — revert StdLib if debugging |
| Slow build | Normal for RELEASE; try `-b DEBUG` or `-b NOOPT` for a faster first link test |

---

## 8. After a green build

You have validated **VS2022 + NASM + AppPkg** on this host.

Next for **Python 3.12.13:** [`../Python-3.12.13/Python312_Windows_VS2022_Build_Guide.md`](../Python-3.12.13/Python312_Windows_VS2022_Build_Guide.md)
(`pyconfig.h`, `Python312.inf`, then `-D BUILD_PYTHON312` / `-D BUILD_PYTHON312_FULL=TRUE`).

---

## 9. CI vs local paths

GitHub Actions checks out **edk2-libc inside the job**, clones **edk2 as a subfolder**, and sets:

```cmd
set PACKAGES_PATH=%CD%\edk2;%CD%
set EDK2_LIBC_PATH=%CD%
```

That is equivalent to sibling clones when `%CD%` is the parent of `edk2` and the libc repo. Your fixed paths in §2 are easier to reuse across sessions.
