#!/usr/bin/env bash
#
# Stage Python 3.12.13 UEFI runtime tree for FAT / QEMU rootfs.
#
# Layout (3.6.8-style; matches PREFIX "\\EFI\\StdLib" in pyconfig.h):
#
#   <OutFolder>/
#     EFI/
#       Tools/Python.efi         # packaged name (build artifact is still Python312.efi)
#       StdLib/lib/python3.12/   # from AppPkg .../Python-3.12.13/Lib
#       StdLib/etc/              # hosts, resolv.conf from StdLib
#
# Usage (from anywhere; needs WORKSPACE = edk2 build root):
#   export EDK2_LIBC_PATH=/path/to/edk2-libc
#   ./create_python_pkg.sh GCC NOOPT X64 /path/to/out
#
set -euo pipefail

TOOL_CHAIN_TAG=${1:-}
TARGET=${2:-}
ARCH=${3:-}
OUT_FOLDER=${4:-}

usage() {
  cat <<EOF

Create Python 3.12 EFI package (Iteration 1).

Usage: $0 <ToolChain> <Target> <Architecture> <OutFolder>
  ToolChain     e.g. GCC, GCC5
  Target        e.g. NOOPT, DEBUG, RELEASE
  Architecture  e.g. X64
  OutFolder     destination directory for the EFI/ tree

EOF
}

error_missing_efi() {
  echo "Failed to create Python EFI package."
  echo "Python312.efi not found under Build/AppPkg/${TARGET}_${TOOL_CHAIN_TAG}/${ARCH}/"
  echo "Build with: build -a ${ARCH} -b ${TARGET} -t ${TOOL_CHAIN_TAG} -p AppPkg/AppPkg.dsc -D BUILD_PYTHON312"
}

if [[ -z "${TOOL_CHAIN_TAG}" || -z "${TARGET}" || -z "${ARCH}" || -z "${OUT_FOLDER}" ]]; then
  usage
  exit 1
fi

if [[ -z "${WORKSPACE:-}" ]]; then
  echo "WORKSPACE is not set (run edksetup.sh first)."
  exit 1
fi

if [[ -z "${EDK2_LIBC_PATH:-}" ]]; then
  echo "Warning: EDK2_LIBC_PATH not set; assuming WORKSPACE contains AppPkg/StdLib."
  EDK2_LIBC_PATH="${WORKSPACE}"
fi

PYTHON_SRC="${EDK2_LIBC_PATH}/AppPkg/Applications/Python/Python-3.12.13"
BUILD_DIR="${WORKSPACE}/Build/AppPkg/${TARGET}_${TOOL_CHAIN_TAG}/${ARCH}"
PYTHON_BIN="${BUILD_DIR}/Python312.efi"

# Fallback: search under BUILD_DIR (EDK embeds the edk2-libc package folder name).
if [[ ! -f "${PYTHON_BIN}" ]]; then
  while IFS= read -r -d '' candidate; do
    case "${candidate}" in
      */Python312/DEBUG/Python312.efi)
        PYTHON_BIN="${candidate}"
        break
        ;;
    esac
  done < <(find "${BUILD_DIR}" -name Python312.efi -print0 2>/dev/null || true)
fi
if [[ ! -f "${PYTHON_BIN}" ]]; then
  while IFS= read -r -d '' candidate; do
    case "${candidate}" in
      */Python312/OUTPUT/Python312.efi)
        PYTHON_BIN="${candidate}"
        break
        ;;
    esac
  done < <(find "${BUILD_DIR}" -name Python312.efi -print0 2>/dev/null || true)
fi
if [[ ! -f "${PYTHON_BIN}" ]]; then
  PYTHON_BIN="$(find "${BUILD_DIR}" -name Python312.efi 2>/dev/null | head -n 1 || true)"
fi

if [[ ! -f "${PYTHON_BIN}" ]]; then
  error_missing_efi
  exit 1
fi

if [[ ! -d "${PYTHON_SRC}/Lib" ]]; then
  echo "Python Lib/ not found at ${PYTHON_SRC}/Lib"
  exit 1
fi

if [[ -f "${PYTHON_SRC}/srcprep.py" ]]; then
  if command -v python3 >/dev/null 2>&1; then
    python3 "${PYTHON_SRC}/srcprep.py"
  elif command -v python >/dev/null 2>&1; then
    python "${PYTHON_SRC}/srcprep.py"
  else
    echo "Warning: host python not found; run srcprep.py manually if PyMod Lib changed."
  fi
fi

BIN_DIR="${OUT_FOLDER}/EFI/Tools"
LIB_DIR="${OUT_FOLDER}/EFI/StdLib/lib/python3.12"
ETC_DIR="${OUT_FOLDER}/EFI/StdLib/etc"

mkdir -p "${BIN_DIR}" "${LIB_DIR}" "${LIB_DIR}/lib-dynload" "${ETC_DIR}"

cp -f "${PYTHON_BIN}" "${BIN_DIR}/Python.efi"

# Prefer rsync (excludes large/unused trees); fall back to cp
if command -v rsync >/dev/null 2>&1; then
  rsync -a \
    --exclude '__pycache__/' \
    --exclude 'test/' \
    --exclude 'tests/' \
    --exclude 'idlelib/' \
    --exclude 'tkinter/' \
    --exclude 'turtledemo/' \
    --exclude 'lib2to3/' \
    "${PYTHON_SRC}/Lib/" "${LIB_DIR}/"
else
  cp -a "${PYTHON_SRC}/Lib/." "${LIB_DIR}/"
  find "${LIB_DIR}" -type d \( -name __pycache__ -o -name test -o -name tests \
    -o -name idlelib -o -name tkinter -o -name turtledemo -o -name lib2to3 \) \
    -prune -exec rm -rf {} + 2>/dev/null || true
fi

# Overlay UEFI-touched Lib from PyMod when present
if [[ -d "${PYTHON_SRC}/PyMod-3.12.13/Lib" ]]; then
  if command -v rsync >/dev/null 2>&1; then
    rsync -a "${PYTHON_SRC}/PyMod-3.12.13/Lib/" "${LIB_DIR}/"
  else
    cp -a "${PYTHON_SRC}/PyMod-3.12.13/Lib/." "${LIB_DIR}/"
  fi
fi

# Phase 8.2: pyreadline vendor (PyMod/Modules/readline → lib/python3.12)
READLINE_VENDOR="${PYTHON_SRC}/PyMod-3.12.13/Modules/readline"
if [[ -d "${READLINE_VENDOR}/pyreadline" && -f "${READLINE_VENDOR}/readline.py" ]]; then
  cp -f "${READLINE_VENDOR}/readline.py" "${LIB_DIR}/readline.py"
  if command -v rsync >/dev/null 2>&1; then
    rsync -a --exclude '__pycache__/' "${READLINE_VENDOR}/pyreadline/" "${LIB_DIR}/pyreadline/"
  else
    rm -rf "${LIB_DIR}/pyreadline"
    cp -a "${READLINE_VENDOR}/pyreadline" "${LIB_DIR}/"
  fi
  echo "Staged pyreadline from ${READLINE_VENDOR}"
else
  echo "Warning: ${READLINE_VENDOR} missing; import readline will fail until Phase 8.2 vendor is present."
fi

if [[ -d "${EDK2_LIBC_PATH}/StdLib/Efi/StdLib/etc" ]]; then
  cp -a "${EDK2_LIBC_PATH}/StdLib/Efi/StdLib/etc/." "${ETC_DIR}/"
fi

ABS_OUT="${OUT_FOLDER}"
if [[ "${OUT_FOLDER}" != /* ]]; then
  ABS_OUT="$(pwd)/${OUT_FOLDER}"
fi

echo
echo "Python 3.12 EFI package ready:"
echo "  ${ABS_OUT}/EFI/Tools/Python.efi"
echo "  ${ABS_OUT}/EFI/StdLib/lib/python3.12/"
echo "  ${ABS_OUT}/EFI/StdLib/etc/"
echo
echo "On UEFI Shell (fs0: = this EFI tree):"
echo "  fs0:"
echo "  cd EFI\\\\Tools"
echo "  Python.efi"
echo
echo "Smoke checks (Phase 8 — vendored ext):"
echo "  import sys; print(sys.version, sys.platform); print(sys.path)"
echo "  import os; print(os.listdir('fs0:\\\\'))"
echo "  import zlib; import readline; import ctypes; import hashlib; import ssl"
echo "  hashlib.sha256(b'x').hexdigest(); ssl.create_default_context()"
