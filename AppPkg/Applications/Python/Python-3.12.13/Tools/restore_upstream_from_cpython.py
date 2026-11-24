#!/usr/bin/env python3
"""Realign stock CPython tree with upstream; keep UEFI deltas in PyMod-3.12.13.

Usage (from Python-3.12.13/):
  python Tools/restore_upstream_from_cpython.py --upstream /path/to/cpython-3.12.13 [--apply]

Without --apply, prints planned actions only.
"""
from __future__ import annotations

import argparse
import os
import shutil
import sys

ROOT_NAME = "PyMod-3.12.13"
SKIP_DIRS = {
    ROOT_NAME,
    ".git",
    "__pycache__",
    "pkg_out",
    "myUEFIPy312",
    "patches",
    "frozen",
}
SCAN_PREFIXES = (
    "Modules/",
    "Python/",
    "Objects/",
    "Include/",
    "Parser/",
    "Programs/",
    "Lib/",
    "Tools/build/",
)
# Entire trees vendored only under PyMod (merge into PyMod, then remove stock copy).
REMOVE_STOCK_DIRS = (
    "efi",  # UEFI glue lives only under PyMod-3.12.13/efi/
    "vs2022_verify",  # migration harness; not shipped
    "Modules/openssl",
    "Modules/zlib",
    "Modules/_ctypes/libffi_msvc",
    "Lib/ssl",
)
# Port-only files under stock; copy to PyMod then remove from stock.
REMOVE_STOCK_FILES_IF_PYMOD = (
    "Include/pyconfig.h",
    "Include/dlfcn.h",
    "Include/pthread.h",
    "Lib/asyncio/uefi_events.py",
    "Lib/ctypes/uefi_pythonapi.py",
    "Lib/uefipath.py",
    "Modules/edk2fault.h",
)
# UEFI-only; not in upstream CPython.
PORT_ONLY_STOCK_FILES = (
    "Tools/build/fix_deepfreeze_latin1.py",
    "Tools/build/fix_deepfreeze_statically_allocated.py",
    "Lib/ctypes/uefi_pythonapi.py",
)
# Co-located headers for PyMod .c (same dir as stock; copy into PyMod when .c is in PyMod).
PYMOD_SIBLING_HEADERS = (
    "Modules/_decimal/docstrings.h",
)


def norm_read(path: str) -> bytes:
    with open(path, "rb") as f:
        return f.read().replace(b"\r\n", b"\n")


def rel_posix(root: str, path: str) -> str:
    return os.path.relpath(path, root).replace("\\", "/")


def should_scan(rel: str) -> bool:
    if rel == ".gitignore":
        return False
    if rel.startswith("Lib/test/") or rel.startswith("Lib/idle_test/"):
        return False
    return any(rel.startswith(p) for p in SCAN_PREFIXES)


def copy_file(src: str, dst: str, dry_run: bool) -> None:
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if dry_run:
        print("COPY", src, "->", dst)
        return
    shutil.copy2(src, dst)


def restore_file(upstream: str, dst: str, dry_run: bool) -> None:
    if dry_run:
        print("RESTORE", dst, "<-", upstream)
        return
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copy2(upstream, dst)


def remove_path(path: str, dry_run: bool) -> None:
    if dry_run:
        print("REMOVE", path)
        return
    if os.path.isdir(path):
        shutil.rmtree(path)
    elif os.path.isfile(path):
        os.remove(path)


def merge_dir_into_pymod(stock_dir: str, pymod_dir: str, dry_run: bool) -> int:
    """Copy vendored files that exist only under stock; never replace an existing PyMod file."""
    actions = 0
    for dirpath, _dirnames, filenames in os.walk(stock_dir):
        for fn in filenames:
            src = os.path.join(dirpath, fn)
            rel = os.path.relpath(src, stock_dir)
            dst = os.path.join(pymod_dir, rel)
            if os.path.isfile(dst):
                continue
            copy_file(src, dst, dry_run)
            actions += 1
    return actions


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--upstream",
        required=True,
        help="Path to extracted upstream CPython v3.12.13 source tree",
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Perform copies/restores (default: dry run)",
    )
    args = parser.parse_args()
    dry_run = not args.apply

    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    upstream = os.path.abspath(args.upstream)
    pymod = os.path.join(root, ROOT_NAME)

    if not os.path.isdir(upstream):
        print("upstream not found:", upstream, file=sys.stderr)
        return 1
    if not os.path.isdir(pymod):
        print("PyMod not found:", pymod, file=sys.stderr)
        return 1

    actions = 0
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for fn in filenames:
            port = os.path.join(dirpath, fn)
            rel = rel_posix(root, port)
            if not should_scan(rel):
                continue
            up = os.path.join(upstream, *rel.split("/"))
            if not os.path.isfile(up):
                continue
            if norm_read(port) == norm_read(up):
                continue
            pymod_path = os.path.join(pymod, *rel.split("/"))
            if not os.path.isfile(pymod_path):
                copy_file(port, pymod_path, dry_run)
                actions += 1
            restore_file(up, port, dry_run)
            actions += 1

    for rel in PORT_ONLY_STOCK_FILES:
        stock = os.path.join(root, *rel.split("/"))
        pymod_path = os.path.join(pymod, *rel.split("/"))
        if not os.path.isfile(stock):
            continue
        if not os.path.isfile(pymod_path) or norm_read(stock) != norm_read(pymod_path):
            copy_file(stock, pymod_path, dry_run)
            actions += 1
        remove_path(stock, dry_run)
        actions += 1

    rel_df = "Python/deepfreeze/deepfreeze.c"
    port_df = os.path.join(root, *rel_df.split("/"))
    pymod_df = os.path.join(pymod, *rel_df.split("/"))
    if os.path.isfile(port_df):
        copy_file(port_df, pymod_df, dry_run)
        actions += 1
        remove_path(port_df, dry_run)
        actions += 1

    for rel in REMOVE_STOCK_DIRS:
        stock = os.path.join(root, *rel.split("/"))
        pymod_copy = os.path.join(pymod, *rel.split("/"))
        if not os.path.isdir(stock):
            continue
        os.makedirs(pymod_copy, exist_ok=True) if not dry_run else None
        actions += merge_dir_into_pymod(stock, pymod_copy, dry_run)
        remove_path(stock, dry_run)
        actions += 1

    for rel in REMOVE_STOCK_FILES_IF_PYMOD:
        stock = os.path.join(root, *rel.split("/"))
        pymod_copy = os.path.join(pymod, *rel.split("/"))
        if not os.path.isfile(stock):
            continue
        if not os.path.isfile(pymod_copy):
            copy_file(stock, pymod_copy, dry_run)
            actions += 1
        if os.path.isfile(pymod_copy):
            remove_path(stock, dry_run)
            actions += 1

    for rel in PYMOD_SIBLING_HEADERS:
        stock = os.path.join(root, *rel.split("/"))
        pymod_path = os.path.join(pymod, *rel.split("/"))
        if os.path.isfile(stock) and not os.path.isfile(pymod_path):
            copy_file(stock, pymod_path, dry_run)
            actions += 1

    # Keep AppPkg/UEFI .gitignore entries (srcprep / PyMod notes). Do not
    # overwrite with stock CPython .gitignore.
    ssl_py_up = os.path.join(upstream, "Lib", "ssl.py")
    ssl_py_stock = os.path.join(root, "Lib", "ssl.py")
    if os.path.isfile(ssl_py_up) and not os.path.isfile(ssl_py_stock):
        # Port uses Lib/ssl/ package under PyMod; stock should keep upstream ssl.py
        # after removing any accidental Lib/ssl/ tree.
        restore_file(ssl_py_up, ssl_py_stock, dry_run)
        actions += 1

    mode = "DRY RUN" if dry_run else "APPLIED"
    print(f"\n{mode}: {actions} action(s).")
    if dry_run:
        print("Re-run with --apply to execute.")
    else:
        print("Next: run srcprep.py to overlay PyMod .h/.py onto the stock tree.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
