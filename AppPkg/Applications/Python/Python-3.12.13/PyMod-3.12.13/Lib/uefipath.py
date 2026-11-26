# Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.
# The full text of the license may be found at
# https://opensource.org/license/bsd-2-clause-patent.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

# Module 'uefipath' -- common operations on UEFI pathnames
"""Common pathname manipulations, UEFI version.

Instead of importing this module directly, import os and refer to this
module as os.path.
"""

# strings representing various path-related bits and pieces
# These are primarily for export; internally, they are hardcoded.
# Should be set before imports for resolving cyclic dependency.
curdir = '.'
pardir = '..'
extsep = '.'
sep = '\\'
pathsep = ';'
altsep = '/'
defpath = '.;fs0:\\EFI\\bin' # UEFI
devnull = 'nul'

import os
import sys
import stat
import genericpath
from genericpath import *


__all__ = ["normcase","isabs","join","splitdrive","splitroot","split","splitext",
           "basename","dirname","commonprefix","getsize","getmtime",
           "getatime","getctime", "islink","exists","lexists","isdir","isfile",
           "ismount", "expanduser","expandvars","normpath","abspath",
           "curdir","pardir","sep","pathsep","defpath","altsep",
           "extsep","devnull","realpath","supports_unicode_filenames","relpath",
           "samefile", "sameopenfile", "samestat", "commonpath", "isjunction"]

def _get_bothseps(path):
    if isinstance(path, bytes):
        return b'\\/'
    else:
        return '\\/'

# Normalize the case of a pathname and map slashes to backslashes.
# Other normalizations (such as optimizing '../' away) are not done
# (this is done by normpath).

try:
    from _winapi import (
        LCMapStringEx as _LCMapStringEx,
        LOCALE_NAME_INVARIANT as _LOCALE_NAME_INVARIANT,
        LCMAP_LOWERCASE as _LCMAP_LOWERCASE)

    def normcase(s):
        """Normalize case of pathname.

        Makes all characters lowercase and all slashes into backslashes.
        """
        s = os.fspath(s)
        if not s:
            return s
        if isinstance(s, bytes):
            encoding = sys.getfilesystemencoding()
            s = s.decode(encoding, 'surrogateescape').replace('/', '\\')
            s = _LCMapStringEx(_LOCALE_NAME_INVARIANT,
                               _LCMAP_LOWERCASE, s)
            return s.encode(encoding, 'surrogateescape')
        else:
            return _LCMapStringEx(_LOCALE_NAME_INVARIANT,
                                  _LCMAP_LOWERCASE,
                                  s.replace('/', '\\'))
except ImportError:
    def normcase(s):
        """Normalize case of pathname.

        Makes all characters lowercase and all slashes into backslashes.
        """
        s = os.fspath(s)
        if isinstance(s, bytes):
            return os.fsencode(os.fsdecode(s).replace('/', '\\').lower())
        return s.replace('/', '\\').lower()


# Return whether a path is absolute.
# Trivial in Posix, harder on Windows.
# For Windows it is absolute if it starts with a slash or backslash (current
# volume), or if a pathname after the volume-letter-and-colon or UNC-resource
# starts with a slash or backslash.

def isabs(s):
    """Test whether a path is absolute"""
    s = os.fspath(s)
    s = splitdrive(s)[1] # UEFI
    return len(s) > 0 and s[0] in _get_bothseps(s)


# Join two (or more) paths.
def join(path, *paths):
    path = os.fspath(path)
    if isinstance(path, bytes):
        sep = b'\\'
        seps = b'\\/'
        colon = b':'
    else:
        sep = '\\'
        seps = '\\/'
        colon = ':'
    try:
        if not paths:
            path[:0] + sep  #23780: Ensure compatible data type even if p is null.
        result_drive, result_root, result_path = splitroot(path)
        for p in map(os.fspath, paths):
            p_drive, p_root, p_path = splitroot(p)
            if p_root:
                # Second path is absolute
                if p_drive or not result_drive:
                    result_drive = p_drive
                result_root = p_root
                result_path = p_path
                continue
            elif p_drive and p_drive != result_drive:
                if p_drive.lower() != result_drive.lower():
                    # Different drives => ignore the first path entirely
                    result_drive = p_drive
                    result_root = p_root
                    result_path = p_path
                    continue
                # Same drive in different case
                result_drive = p_drive
            # Second path is relative to the first
            if result_path and result_path[-1] not in seps:
                result_path = result_path + sep
            result_path = result_path + p_path
        ## add separator between UNC and non-absolute path
        if (result_path and not result_root and
            result_drive and result_drive[-1:] not in seps):
            return result_drive + sep + result_path
        return result_drive + result_root + result_path
    except (TypeError, AttributeError, BytesWarning):
        genericpath._check_arg_types('join', path, *paths)
        raise


# Split a path in a drive specification (a drive letter followed by a
# colon) and the path specification.
# It is always true that drivespec + pathspec == p
def splitdrive(p):
    """Split a pathname into drive/UNC sharepoint and relative path specifiers.
    Returns a 2-tuple (drive_or_unc, path); either part may be empty.

    If you assign
        result = splitdrive(p)
    It is always true that:
        result[0] + result[1] == p

    If the path contained a drive letter, drive_or_unc will contain everything
    up to and including the colon.  e.g. splitdrive("c:/dir") returns ("c:", "/dir")

    If the path contained a UNC path, the drive_or_unc will contain the host name
    and share up to but not including the fourth directory separator character.
    e.g. splitdrive("//host/computer/dir") returns ("//host/computer", "/dir")

    Paths cannot contain both a drive letter and a UNC path.

    """
    drive, root, tail = splitroot(p)
    return drive, root + tail


def splitroot(p):
    """Split a pathname into drive, root and tail. The drive is defined
    exactly as in splitdrive(). On Windows, the root may be a single path
    separator or an empty string. The tail contains anything after the root.
    For example:

        splitroot('//server/share/') == ('//server/share', '/', '')
        splitroot('C:/Users/Barney') == ('C:', '/', 'Users/Barney')
        splitroot('C:///spam///ham') == ('C:', '/', '//spam///ham')
        splitroot('Windows/notepad') == ('', '', 'Windows/notepad')
    """
    p = os.fspath(p)
    if isinstance(p, bytes):
        sep = b'\\'
        altsep = b'/'
        colon = b':'
        unc_prefix = b'\\\\?\\UNC\\'
        empty = b''
    else:
        sep = '\\'
        altsep = '/'
        colon = ':'
        unc_prefix = '\\\\?\\UNC\\'
        empty = ''
    normp = p.replace(altsep, sep)
    if normp[:1] == sep:
        if normp[1:2] == sep:
            # UNC drives, e.g. \\server\share or \\?\UNC\server\share
            # Device drives, e.g. \\.\device or \\?\device
            start = 8 if normp[:8].upper() == unc_prefix else 2
            index = normp.find(sep, start)
            if index == -1:
                return p, empty, empty
            index2 = normp.find(sep, index + 1)
            if index2 == -1:
                return p, empty, empty
            return p[:index2], p[index2:index2 + 1], p[index2 + 1:]
        else:
            # Relative path with root, e.g. \Windows
            return empty, p[:1], p[1:]
    elif (pos := normp.find(colon)) > 0 :
        if normp[pos+1:pos+2] == sep:
            # Absolute drive-letter path, e.g. X:\Windows
            return p[:pos+1], p[pos+1:pos+2], p[pos+2:]
        else:
            # Relative path with drive, e.g. X:Windows
            return p[:pos+1], empty, p[pos+1:]
    else:
        # Relative path, e.g. Windows
        return empty, empty, p


# Split a path in head (everything up to the last '/') and tail (the
# rest).  After the trailing '/' is stripped, the invariant
# join(head, tail) == p holds.
# The resulting head won't end in '/' unless it is the root.

def split(p):
    """Split a pathname.

    Return tuple (head, tail) where tail is everything after the final slash.
    Either part may be empty."""
    p = os.fspath(p)
    seps = _get_bothseps(p)
    d, r, p = splitroot(p)
    # set i to index beyond p's last slash
    i = len(p)
    while i and p[i-1] not in seps:
        i -= 1
    head, tail = p[:i], p[i:]  # now tail has no slashes
    return d + r + head.rstrip(seps), tail


# Split a path in root and extension.
# The extension is everything starting at the last dot in the last
# pathname component; the root is everything before that.
# It is always true that root + ext == p.

def splitext(p):
    p = os.fspath(p)
    if isinstance(p, bytes):
        return genericpath._splitext(p, b'\\', b'/', b'.')
    else:
        return genericpath._splitext(p, '\\', '/', '.')
splitext.__doc__ = genericpath._splitext.__doc__


# Return the tail (basename) part of a path.

def basename(p):
    """Returns the final component of a pathname"""
    return split(p)[1]


# Return the head (dirname) part of a path.

def dirname(p):
    """Returns the directory component of a pathname"""
    return split(p)[0]


# Is a path a junction?

if hasattr(os.stat_result, 'st_reparse_tag'):
    def isjunction(path):
        """Test whether a path is a junction"""
        try:
            st = os.lstat(path)
        except (OSError, ValueError, AttributeError):
            return False
        return bool(st.st_reparse_tag == stat.IO_REPARSE_TAG_MOUNT_POINT)
else:
    def isjunction(path):
        """Test whether a path is a junction"""
        os.fspath(path)
        return False


# Being true for dangling symbolic links is also useful.

def lexists(path):
    """Test whether a path exists.  Returns True for broken symbolic links"""
    try:
        st = os.lstat(path)
    except (OSError, ValueError):
        return False
    return True

# Is a path a mount point?
# Any drive letter root (eg c:\)
# Any share UNC (eg \\server\share)
# Any volume mounted on a filesystem folder
#
# No one method detects all three situations. Historically we've lexically
# detected drive letter roots and share UNCs. The canonical approach to
# detecting mounted volumes (querying the reparse tag) fails for the most
# common case: drive letter roots. The alternative which uses GetVolumePathName
# fails if the drive letter is the result of a SUBST.
_getvolumepathname = None
def ismount(path):
    """Test whether a path is a mount point (a drive root, the root of a
    share, or a mounted volume)"""
    path = os.fspath(path)
    seps = _get_bothseps(path)
    path = abspath(path)
    drive, root, rest = splitroot(path)
    if drive and drive[0] in seps:
        return not rest
    if root and not rest:
        return True

    if _getvolumepathname:
        x = path.rstrip(seps)
        y =_getvolumepathname(path).rstrip(seps)
        return x.casefold() == y.casefold()
    else:
        return False


# Expand paths beginning with '~' or '~user'.
# '~' means $HOME; '~user' means that user's home directory.
# If the path doesn't begin with '~', or if the user or $HOME is unknown,
# the path is returned unchanged (leaving error reporting to whatever
# function is called with the expanded path as argument).
# See also module 'glob' for expansion of *, ? and [...] in pathnames.
# (A function should also be defined to do full *sh-style environment
# variable expansion.)

def expanduser(path):
    """Expand ~ and ~user constructs.

    If user or $HOME is unknown, do nothing."""
    path = os.fspath(path)
    if isinstance(path, bytes):
        tilde = b'~'
    else:
        tilde = '~'
    if not path.startswith(tilde):
        return path
    i, n = 1, len(path)
    while i < n and path[i] not in _get_bothseps(path):
        i += 1

    if 'USERPROFILE' in os.environ:
        userhome = os.environ['USERPROFILE']
    elif not 'HOMEPATH' in os.environ:
        return path
    else:
        try:
            drive = os.environ['HOMEDRIVE']
        except KeyError:
            drive = ''
        userhome = join(drive, os.environ['HOMEPATH'])

    if i != 1: #~user
        target_user = path[1:i]
        if isinstance(target_user, bytes):
            target_user = os.fsdecode(target_user)
        current_user = os.environ.get('USERNAME')

        if target_user != current_user:
            # Try to guess user home directory.  By default all user
            # profile directories are located in the same place and are
            # named by corresponding usernames.  If userhome isn't a
            # normal profile directory, this guess is likely wrong,
            # so we bail out.
            if current_user != basename(userhome):
                return path
            userhome = join(dirname(userhome), target_user)

    if isinstance(path, bytes):
        userhome = os.fsencode(userhome)

    return userhome + path[i:]


# Expand paths containing shell variable substitutions.
# The following rules apply:
#       - no expansion within single quotes
#       - '$$' is translated into '$'
#       - '%%' is translated into '%' if '%%' are not seen in %var1%%var2%
#       - ${varname} is accepted.
#       - $varname is accepted.
#       - %varname% is accepted.
#       - varnames can be made out of letters, digits and the characters '_-'
#         (though is not verified in the ${varname} and %varname% cases)
# XXX With COMMAND.COM you can use any characters in a variable name,
# XXX except '^|<>='.

def expandvars(path):
    """Expand shell variables of the forms $var, ${var} and %var%.

    Unknown variables are left unchanged."""
    path = os.fspath(path)
    if isinstance(path, bytes):
        if b'$' not in path and b'%' not in path:
            return path
        import string
        varchars = bytes(string.ascii_letters + string.digits + '_-', 'ascii')
        quote = b'\''
        percent = b'%'
        brace = b'{'
        rbrace = b'}'
        dollar = b'$'
        environ = getattr(os, 'environb', None)
    else:
        if '$' not in path and '%' not in path:
            return path
        import string
        varchars = string.ascii_letters + string.digits + '_-'
        quote = '\''
        percent = '%'
        brace = '{'
        rbrace = '}'
        dollar = '$'
        environ = os.environ
    res = path[:0]
    index = 0
    pathlen = len(path)
    while index < pathlen:
        c = path[index:index+1]
        if c == quote:   # no expansion within single quotes
            path = path[index + 1:]
            pathlen = len(path)
            try:
                index = path.index(c)
                res += c + path[:index + 1]
            except ValueError:
                res += c + path
                index = pathlen - 1
        elif c == percent:  # variable or '%'
            if path[index + 1:index + 2] == percent:
                res += c
                index += 1
            else:
                path = path[index+1:]
                pathlen = len(path)
                try:
                    index = path.index(percent)
                except ValueError:
                    res += percent + path
                    index = pathlen - 1
                else:
                    var = path[:index]
                    try:
                        if environ is None:
                            value = os.fsencode(os.environ[os.fsdecode(var)])
                        else:
                            value = environ[var]
                    except KeyError:
                        value = percent + var + percent
                    res += value
        elif c == dollar:  # variable or '$$'
            if path[index + 1:index + 2] == dollar:
                res += c
                index += 1
            elif path[index + 1:index + 2] == brace:
                path = path[index+2:]
                pathlen = len(path)
                try:
                    index = path.index(rbrace)
                except ValueError:
                    res += dollar + brace + path
                    index = pathlen - 1
                else:
                    var = path[:index]
                    try:
                        if environ is None:
                            value = os.fsencode(os.environ[os.fsdecode(var)])
                        else:
                            value = environ[var]
                    except KeyError:
                        value = dollar + brace + var + rbrace
                    res += value
            else:
                var = path[:0]
                index += 1
                c = path[index:index + 1]
                while c and c in varchars:
                    var += c
                    index += 1
                    c = path[index:index + 1]
                try:
                    if environ is None:
                        value = os.fsencode(os.environ[os.fsdecode(var)])
                    else:
                        value = environ[var]
                except KeyError:
                    value = dollar + var
                res += value
                if c:
                    index -= 1
        else:
            res += c
        index += 1
    return res


# Normalize a path, e.g. A//B, A/./B and A/foo/../B all become A\B.
# Previously, this function also truncated pathnames to 8+3 format,
# but as this module is called "ntpath", that's obviously wrong!
def normpath(path):
    """Normalize path, eliminating double slashes, etc."""
    path = os.fspath(path)
    if isinstance(path, bytes):
        sep = b'\\'
        altsep = b'/'
        curdir = b'.'
        pardir = b'..'
    else:
        sep = '\\'
        altsep = '/'
        curdir = '.'
        pardir = '..'
    path = path.replace(altsep, sep)
    drive, root, path = splitroot(path)
    prefix = drive + root
    comps = path.split(sep)
    i = 0
    while i < len(comps):
        if not comps[i] or comps[i] == curdir:
            del comps[i]
        elif comps[i] == pardir:
            if i > 0 and comps[i-1] != pardir:
                del comps[i-1:i+1]
                i -= 1
            elif i == 0 and root:
                del comps[i]
            else:
                i += 1
        else:
            i += 1
    # If the path is now empty, substitute '.'
    if not prefix and not comps:
        comps.append(curdir)
    return prefix + sep.join(comps)

def _abspath_fallback(path):
    """Return the absolute version of a path as a fallback function in case
    `nt._getfullpathname` is not available or raises OSError. See bpo-31047 for
    more.

    """

    path = os.fspath(path)
    if not isabs(path):
        if isinstance(path, bytes):
            cwd = os.getcwdb()
        else:
            cwd = os.getcwd()
        path = join(cwd, path)
    return normpath(path)

abspath = _abspath_fallback
realpath = abspath

# All supported version have Unicode filename support.
supports_unicode_filenames = True

def relpath(path, start=None):
    """Return a relative version of a path"""
    path = os.fspath(path)
    if isinstance(path, bytes):
        sep = b'\\'
        curdir = b'.'
        pardir = b'..'
    else:
        sep = '\\'
        curdir = '.'
        pardir = '..'

    if start is None:
        start = curdir

    if not path:
        raise ValueError("no path specified")

    start = os.fspath(start)
    try:
        start_abs = abspath(normpath(start))
        path_abs = abspath(normpath(path))
        start_drive, _, start_rest = splitroot(start_abs)
        path_drive, _, path_rest = splitroot(path_abs)
        if normcase(start_drive) != normcase(path_drive):
            raise ValueError("path is on mount %r, start on mount %r" % (
                path_drive, start_drive))

        start_list = [x for x in start_rest.split(sep) if x]
        path_list = [x for x in path_rest.split(sep) if x]
        # Work out how much of the filepath is shared by start and path.
        i = 0
        for e1, e2 in zip(start_list, path_list):
            if normcase(e1) != normcase(e2):
                break
            i += 1

        rel_list = [pardir] * (len(start_list)-i) + path_list[i:]
        if not rel_list:
            return curdir
        return join(*rel_list)
    except (TypeError, ValueError, AttributeError, BytesWarning, DeprecationWarning):
        genericpath._check_arg_types('relpath', path, start)
        raise


# Return the longest common sub-path of the sequence of paths given as input.
# The function is case-insensitive and 'separator-insensitive', i.e. if the
# only difference between two paths is the use of '\' versus '/' as separator,
# they are deemed to be equal.
#
# However, the returned path will have the standard '\' separator (even if the
# given paths had the alternative '/' separator) and will have the case of the
# first path given in the sequence. Additionally, any trailing separator is
# stripped from the returned path.

def commonpath(paths):
    """Given a sequence of path names, returns the longest common sub-path."""

    if not paths:
        raise ValueError('commonpath() arg is an empty sequence')

    paths = tuple(map(os.fspath, paths))
    if isinstance(paths[0], bytes):
        sep = b'\\'
        altsep = b'/'
        curdir = b'.'
    else:
        sep = '\\'
        altsep = '/'
        curdir = '.'

    try:
        drivesplits = [splitroot(p.replace(altsep, sep).lower()) for p in paths]
        split_paths = [p.split(sep) for d, r, p in drivesplits]

        if len({r for d, r, p in drivesplits}) != 1:
            raise ValueError("Can't mix absolute and relative paths")

        # Check that all drive letters or UNC paths match. The check is made only
        # now otherwise type errors for mixing strings and bytes would not be
        # caught.
        if len({d for d, r, p in drivesplits}) != 1:
            raise ValueError("Paths don't have the same drive")

        drive, root, path = splitroot(paths[0].replace(altsep, sep))
        common = path.split(sep)
        common = [c for c in common if c and c != curdir]

        split_paths = [[c for c in s if c and c != curdir] for s in split_paths]
        s1 = min(split_paths)
        s2 = max(split_paths)
        for i, c in enumerate(s1):
            if c != s2[i]:
                common = common[:i]
                break
        else:
            common = common[:len(s1)]

        return drive + root + sep.join(common)
    except (TypeError, AttributeError):
        genericpath._check_arg_types('commonpath', *paths)
        raise
