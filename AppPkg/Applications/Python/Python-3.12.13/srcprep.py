"""Copy PyMod-3.12.13 header and Python files into the CPython tree.

Mirrors AppPkg Python-3.6.8/srcprep.py. Only .h and .py are copied;
.C sources under PyMod are referenced directly from Python312.inf.

Skipped under PyMod (not overlaid): Modules/readline/, Modules/libffi/,
Modules/zlib/, Modules/openssl/ — vendored trees used only at link/stage time.

Also skipped: Python/frozen_modules/ — frozen marshal headers and frozen.c
are built from PyMod-3.12.13/Python/ (see Python312.inf); stock Python/frozen_modules/
stays upstream-clean (README only).
"""
import os
import shutil
import stat


def copyDirTree(root_src_dir, root_dst_dir):
    """
    Copy directory tree. Overwrites also read only files.
    :param root_src_dir: source directory
    :param root_dst_dir: destination directory
    """
    for src_dir, _dirs, files in os.walk(root_src_dir):
        dst_dir = src_dir.replace(root_src_dir, root_dst_dir, 1)
        # Vendored pyreadline lives under Modules/readline/; package script stages it.
        if os.path.normpath(dst_dir).replace("\\", "/").endswith("Modules/readline"):
            continue
        if "/Modules/readline/" in os.path.normpath(dst_dir).replace("\\", "/"):
            continue
        norm_dst = os.path.normpath(dst_dir).replace("\\", "/")
        if norm_dst.endswith("Modules/libffi") or "/Modules/libffi/" in norm_dst:
            continue
        if norm_dst.endswith("Python/frozen_modules") or "/Python/frozen_modules/" in norm_dst:
            continue
        if not os.path.exists(dst_dir):
            os.makedirs(dst_dir)
        for file_ in files:
            src_file = os.path.join(src_dir, file_)
            dst_file = os.path.join(dst_dir, file_)
            if ".h" in src_file or ".py" in src_file:
                if os.path.exists(dst_file):
                    try:
                        os.remove(dst_file)
                    except PermissionError:
                        os.chmod(dst_file, stat.S_IWUSR)
                        os.remove(dst_file)
                shutil.copy(src_file, dst_dir)


if __name__ == "__main__":
    src = r"PyMod-3.12.13"
    dest = os.getcwd()
    copyDirTree(src, dest)
    print("srcprep: copied .h/.py from %s into %s" % (src, dest))
