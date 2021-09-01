"""Python module to copy specific file to respective destination folder"""
import os
import shutil
 
def copyDirTree(root_src_dir,root_dst_dir):
    """
    Copy directory tree. Overwrites also read only files.
    :param root_src_dir: source directory
    :param root_dst_dir:  destination directory
    """
    for src_dir, dirs, files in os.walk(root_src_dir):
        dst_dir = src_dir.replace(root_src_dir, root_dst_dir, 1)
        if not os.path.exists(dst_dir):
            os.makedirs(dst_dir)
        for file_ in files:
            src_file = os.path.join(src_dir, file_)
            dst_file = os.path.join(dst_dir, file_)
            if(src_file.__contains__('.h') or src_file.__contains__('.py')):
                if os.path.exists(dst_file):
                    try:
                        os.remove(dst_file)
                    except PermissionError as exc:
                        os.chmod(dst_file, stat.S_IWUSR)
                        os.remove(dst_file)
 
                shutil.copy(src_file, dst_dir)
 
src = r'PyMod-3.6.8'
dest = os.getcwd()
copyDirTree(src,dest)
