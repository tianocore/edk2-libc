/* Return the initial module search path. */

#include "Python.h"
#include  <osdefs.h>
#include  <ctype.h>

//#include <sys/types.h>
//#include <string.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

/* Search in some common locations for the associated Python libraries.
 *
 * Two directories must be found, the platform independent directory
 * (prefix), containing the common .py and .pyc files, and the platform
 * dependent directory (exec_prefix), containing the shared library
 * modules.  Note that prefix and exec_prefix can be the same directory,
 * but for some installations, they are different.
 *
 * Py_GetPath() carries out separate searches for prefix and exec_prefix.
 * Each search tries a number of different locations until a ``landmark''
 * file or directory is found.  If no prefix or exec_prefix is found, a
 * warning message is issued and the preprocessor defined PREFIX and
 * EXEC_PREFIX are used (even though they will not work); python carries on
 * as best as is possible, but most imports will fail.
 *
 * Before any searches are done, the location of the executable is
 * determined.  If argv[0] has one or more slashes in it, it is used
 * unchanged.  Otherwise, it must have been invoked from the shell's path,
 * so we search $PATH for the named executable and use that.  If the
 * executable was not found on $PATH (or there was no $PATH environment
 * variable), the original argv[0] string is used.
 *
 * Next, the executable location is examined to see if it is a symbolic
 * link.  If so, the link is chased (correctly interpreting a relative
 * pathname if one is found) and the directory of the link target is used.
 *
 * Finally, argv0_path is set to the directory containing the executable
 * (i.e. the last component is stripped).
 *
 * With argv0_path in hand, we perform a number of steps.  The same steps
 * are performed for prefix and for exec_prefix, but with a different
 * landmark.
 *
 * Step 1. Are we running python out of the build directory?  This is
 * checked by looking for a different kind of landmark relative to
 * argv0_path.  For prefix, the landmark's path is derived from the VPATH
 * preprocessor variable (taking into account that its value is almost, but
 * not quite, what we need).  For exec_prefix, the landmark is
 * pybuilddir.txt.  If the landmark is found, we're done.
 *
 * For the remaining steps, the prefix landmark will always be
 * lib/python$VERSION/os.py and the exec_prefix will always be
 * lib/python$VERSION/lib-dynload, where $VERSION is Python's version
 * number as supplied by the Makefile.  Note that this means that no more
 * build directory checking is performed; if the first step did not find
 * the landmarks, the assumption is that python is running from an
 * installed setup.
 *
 * Step 2. See if the $PYTHONHOME environment variable points to the
 * installed location of the Python libraries.  If $PYTHONHOME is set, then
 * it points to prefix and exec_prefix.  $PYTHONHOME can be a single
 * directory, which is used for both, or the prefix and exec_prefix
 * directories separated by a colon.
 *
 * Step 3. Try to find prefix and exec_prefix relative to argv0_path,
 * backtracking up the path until it is exhausted.  This is the most common
 * step to succeed.  Note that if prefix and exec_prefix are different,
 * exec_prefix is more likely to be found; however if exec_prefix is a
 * subdirectory of prefix, both will be found.
 *
 * Step 4. Search the directories pointed to by the preprocessor variables
 * PREFIX and EXEC_PREFIX.  These are supplied by the Makefile but can be
 * passed in as options to the configure script.
 *
 * That's it!
 *
 * Well, almost.  Once we have determined prefix and exec_prefix, the
 * preprocessor variable PYTHONPATH is used to construct a path.  Each
 * relative path on PYTHONPATH is prefixed with prefix.  Then the directory
 * containing the shared library modules is appended.  The environment
 * variable $PYTHONPATH is inserted in front of it all.  Finally, the
 * prefix and exec_prefix globals are tweaked so they reflect the values
 * expected by other code, by stripping the "lib/python$VERSION/..." stuff
 * off.  If either points to the build directory, the globals are reset to
 * the corresponding preprocessor variables (so sys.prefix will reflect the
 * installation location, even though sys.path points into the build
 * directory).  This seems to make more sense given that currently the only
 * known use of sys.prefix and sys.exec_prefix is for the ILU installation
 * process to find the installed Python tree.
 *
 * An embedding application can use Py_SetPath() to override all of
 * these authomatic path computations.
 *
 * NOTE: Windows MSVC builds use PC/getpathp.c instead!
 */

#ifdef __cplusplus
 extern "C" {
#endif

/* Filename separator */
#ifndef SEP
#define SEP     L'/'
#define ALTSEP  L'\\'
#endif

#ifndef ALTSEP
#define ALTSEP  L'\\'
#endif


#define SIFY_I( x ) L"" #x
#define SIFY( y )   SIFY_I( y )

#ifndef PREFIX
  #define PREFIX      L"/Efi/StdLib"
#endif

#ifndef EXEC_PREFIX
  #define EXEC_PREFIX      PREFIX
#endif

#ifndef   LIBPYTHON
  #define   LIBPYTHON     L"lib/python" VERSION L"." SIFY(PY_MICRO_VERSION)
#endif

#ifndef PYTHONPATH
  #define PYTHONPATH  LIBPYTHON
#endif

#ifndef LANDMARK
  #define LANDMARK    L"os.py"
#endif

#ifndef VERSION
  #define VERSION     SIFY(PY_MAJOR_VERSION) SIFY(PY_MINOR_VERSION)
#endif

#ifndef VPATH
  #define VPATH       L"."
#endif

/* Search path entry delimiter */
//#   define DELIM      ';'
#   define DELIM_STR  ";"

#ifdef DELIM
  #define sDELIM        L";"
#endif


#if !defined(PREFIX) || !defined(EXEC_PREFIX) || !defined(VERSION) || !defined(VPATH)
#error "PREFIX, EXEC_PREFIX, VERSION, and VPATH must be constant defined"
#endif

#ifndef LANDMARK
#define LANDMARK L"os.py"
#endif

static wchar_t prefix[MAXPATHLEN+1] = {0};
static wchar_t exec_prefix[MAXPATHLEN+1] = {0};
static wchar_t progpath[MAXPATHLEN+1] = {0};
static wchar_t *module_search_path = NULL;
static wchar_t   lib_python[]             = LIBPYTHON;
static wchar_t   volume_name[32]          = { 0 };


/* Get file status. Encode the path to the locale encoding. */

static int
_Py_wstat(const wchar_t* path, struct stat *buf)
{
    int err;
    char *fname;
    fname = Py_EncodeLocale(path, NULL);
    if (fname == NULL) {
        errno = EINVAL;
        return -1;
    }
    err = stat(fname, buf);
    PyMem_Free(fname);
    return err;
}

/* Get file status. Encode the path to the locale encoding. */

static wchar_t *
_Py_basename(const wchar_t* path)
{
    int err;
	size_t len = 0;
    char *fname, *buf;
    wchar_t *bname; 
    fname = Py_EncodeLocale(path, NULL);
    if (fname == NULL) {
        errno = EINVAL;
        return NULL;
    }
    buf = basename(fname);
    PyMem_Free(fname);
    len = strlen(buf);
    bname = Py_DecodeLocale(buf, &len);
    return bname;
}

/** Determine if "ch" is a separator character.

    @param[in]  ch      The character to test.

    @retval     TRUE    ch is a separator character.
    @retval     FALSE   ch is NOT a separator character.
**/
static int
is_sep(wchar_t ch)
{
  return ch == SEP || ch == ALTSEP;
}

/** Determine if a path is absolute, or not.
    An absolute path consists of a volume name, "VOL:", followed by a rooted path,
    "/path/elements".  If both of these components are present, the path is absolute.

    Let P be a pointer to the path to test.
    Let A be a pointer to the first ':' in P.
    Let B be a pointer to the first '/' or '\\' in P.

    If A and B are not NULL
      If (A-P+1) == (B-P) then the path is absolute.
    Otherwise, the path is NOT absolute.

    @param[in]  path    The path to test.

    @retval     -1      Path is absolute but lacking volume name.
    @retval      0      Path is NOT absolute.
    @retval      1      Path is absolute.
*/
static int
is_absolute(wchar_t *path)
{
  wchar_t  *A;
  wchar_t  *B;

  A = wcschr(path, L':');
  B = wcspbrk(path, L"/\\");

  if(B != NULL) {
    if(A == NULL) {
      if(B == path) {
        return -1;
      }
    }
    else {
      if(((A - path) + 1) == (B - path)) {
        return 1;
      }
    }
  }
  return 0;
}

static void
reduce(wchar_t *dir)
{
    size_t i = wcslen(dir);
    while (i > 0 && !is_sep(dir[i]))
        --i;
    dir[i] = '\0';
}

static int
isfile(wchar_t *filename)          /* Is file, not directory */
{
    struct stat buf;
    if (_Py_wstat(filename, &buf) != 0)
        return 0;
    if (!S_ISREG(buf.st_mode))
        return 0;
    return 1;
}


static int
ismodule(wchar_t *filename)        /* Is module -- check for .pyc too */
{
    if (isfile(filename))
        return 1;

    /* Check for the compiled version of prefix. */
    if (wcslen(filename) < MAXPATHLEN) {
        wcscat(filename, L"c");
        if (isfile(filename))
            return 1;
    }
    return 0;
}

static int
isdir(wchar_t *filename)                   /* Is directory */
{
    struct stat buf;
    if (_Py_wstat(filename, &buf) != 0)
        return 0;
    if (!S_ISDIR(buf.st_mode))
        return 0;
    return 1;
}

/* Add a path component, by appending stuff to buffer.
   buffer must have at least MAXPATHLEN + 1 bytes allocated, and contain a
   NUL-terminated string with no more than MAXPATHLEN characters (not counting
   the trailing NUL).  It's a fatal error if it contains a string longer than
   that (callers must be careful!).  If these requirements are met, it's
   guaranteed that buffer will still be a NUL-terminated string with no more
   than MAXPATHLEN characters at exit.  If stuff is too long, only as much of
   stuff as fits will be appended.
*/
static void
joinpath(wchar_t *buffer, wchar_t *stuff)
{
    size_t n, k;
    k = 0;
    if (is_absolute(stuff) == 1){
        n = 0;
    }
    else {
        n = wcslen(buffer);
        if (n == 0) { 
			wcsncpy(buffer, volume_name, MAXPATHLEN);
			n = wcslen(buffer);
		}
        if (n > 0  && n < MAXPATHLEN){
	      if(!is_sep(buffer[n-1])) {
				buffer[n++] = SEP;
			}
		   if(is_sep(stuff[0]))   ++stuff;
		}
    }
    if (n > MAXPATHLEN)
        Py_FatalError("buffer overflow in getpath.c's joinpath()");
    k = wcslen(stuff);
    if (n + k > MAXPATHLEN)
        k = MAXPATHLEN - n;
    wcsncpy(buffer+n, stuff, k);
    buffer[n+k] = '\0';
}

static int
isxfile(wchar_t *filename)
{
    struct stat  buf;
    wchar_t        *bn;
    wchar_t        *newbn;
    int          bnlen; 
    char         *filename_str; 
	
    bn = _Py_basename(filename);            // Separate off the file name component
    reduce(filename);                   // and isolate the path component
    bnlen = wcslen(bn);
    newbn = wcsrchr(bn, L'.');           // Does basename contain a period?
    if(newbn == NULL) {                   // Does NOT contain a period.
      newbn = &bn[bnlen];
      wcsncpy(newbn, L".efi", MAXPATHLEN - bnlen);    // append ".efi" to basename
      bnlen += 4;
    }
    else if(wcscmp(newbn, L".efi") != 0) {
      return 0;                         // File can not be executable.
    }
    joinpath(filename, bn);             // Stitch path and file name back together

    return isdir(filename);
}

/* copy_absolute requires that path be allocated at least
   MAXPATHLEN + 1 bytes and that p be no more than MAXPATHLEN bytes. */
static void
copy_absolute(wchar_t *path, wchar_t *p, size_t pathlen)
{
    if (is_absolute(p) == 1)
        wcscpy(path, p);
    else {
        if (!_Py_wgetcwd(path, pathlen)) {
            /* unable to get the current directory */
			  if(volume_name[0] != 0) {
				wcscpy(path, volume_name);
				joinpath(path, p);
			  }
			  else
				wcscpy(path, p);
            return;
        }
        if (p[0] == '.' && p[1] == SEP)
            p += 2;
        joinpath(path, p);
    }
}

/* absolutize() requires that path be allocated at least MAXPATHLEN+1 bytes. */
static void
absolutize(wchar_t *path)
{
    wchar_t buffer[MAXPATHLEN+1];

    if (is_absolute(path) == 1)
        return;
    copy_absolute(buffer, path, MAXPATHLEN+1);
    wcscpy(path, buffer);
}

/** Extract the volume name from a path.

    @param[out]   Dest    Pointer to location in which to store the extracted volume name.
    @param[in]    path    Pointer to the path to extract the volume name from.
**/
static void
set_volume(wchar_t *Dest, wchar_t *path)
{
  size_t    VolLen;

  if(is_absolute(path)) {
    VolLen = wcscspn(path, L"/\\:");
    if((VolLen != 0) && (path[VolLen] == L':')) {
      (void) wcsncpy(Dest, path, VolLen + 1);
    }
  }
}


/* search for a prefix value in an environment file. If found, copy it
   to the provided buffer, which is expected to be no more than MAXPATHLEN
   bytes long.
*/

static int
find_env_config_value(FILE * env_file, const wchar_t * key, wchar_t * value)
{
    int result = 0; /* meaning not found */
    char buffer[MAXPATHLEN*2+1];  /* allow extra for key, '=', etc. */

    fseek(env_file, 0, SEEK_SET);
    while (!feof(env_file)) {
        char * p = fgets(buffer, MAXPATHLEN*2, env_file);
        wchar_t tmpbuffer[MAXPATHLEN*2+1];
        PyObject * decoded;
        int n;

        if (p == NULL)
            break;
        n = strlen(p);
        if (p[n - 1] != '\n') {
            /* line has overflowed - bail */
            break;
        }
        if (p[0] == '#')    /* Comment - skip */
            continue;
        decoded = PyUnicode_DecodeUTF8(buffer, n, "surrogateescape");
        if (decoded != NULL) {
            Py_ssize_t k;
            wchar_t * state;
            k = PyUnicode_AsWideChar(decoded,
                                     tmpbuffer, MAXPATHLEN * 2);
            Py_DECREF(decoded);
            if (k >= 0) {
                wchar_t * tok = wcstok(tmpbuffer, L" \t\r\n", &state);
                if ((tok != NULL) && !wcscmp(tok, key)) {
                    tok = wcstok(NULL, L" \t", &state);
                    if ((tok != NULL) && !wcscmp(tok, L"=")) {
                        tok = wcstok(NULL, L"\r\n", &state);
                        if (tok != NULL) {
                            wcsncpy(value, tok, MAXPATHLEN);
                            result = 1;
                            break;
                        }
                    }
                }
            }
        }
    }
    return result;
}

/* search_for_prefix requires that argv0_path be no more than MAXPATHLEN
   bytes long.
*/
static int
search_for_prefix(wchar_t *argv0_path, wchar_t *home, wchar_t *_prefix,
                  wchar_t *lib_python)
{
    size_t n;
    wchar_t *vpath;

    /* If PYTHONHOME is set, we believe it unconditionally */
    if (home) {
        wchar_t *delim;
        wcsncpy(prefix, home, MAXPATHLEN);
        prefix[MAXPATHLEN] = L'\0';
        delim = wcschr(prefix, DELIM);
        if (delim)
            *delim = L'\0';
        joinpath(prefix, lib_python);
        joinpath(prefix, LANDMARK);
        return 1;
    }

    /* Check to see if argv[0] is in the build directory */
    wcsncpy(prefix, argv0_path, MAXPATHLEN);
    prefix[MAXPATHLEN] = L'\0';
    joinpath(prefix, L"Modules/Setup");
    if (isfile(prefix)) {
        /* Check VPATH to see if argv0_path is in the build directory. */
        vpath = Py_DecodeLocale(VPATH, NULL);
        if (vpath != NULL) {
            wcsncpy(prefix, argv0_path, MAXPATHLEN);
            prefix[MAXPATHLEN] = L'\0';
            joinpath(prefix, vpath);
            PyMem_RawFree(vpath);
            joinpath(prefix, L"Lib");
            joinpath(prefix, LANDMARK);
            if (ismodule(prefix))
                return -1;
        }
    }

    /* Search from argv0_path, until root is found */
    copy_absolute(prefix, argv0_path, MAXPATHLEN+1);
    do {
        n = wcslen(prefix);
        joinpath(prefix, lib_python);
        joinpath(prefix, LANDMARK);
        if (ismodule(prefix))
            return 1;
        prefix[n] = L'\0';
        reduce(prefix);
    } while (prefix[0]);

    /* Look at configure's PREFIX */
    wcsncpy(prefix, _prefix, MAXPATHLEN);
    prefix[MAXPATHLEN] = L'\0';
    joinpath(prefix, lib_python);
    joinpath(prefix, LANDMARK);
    if (ismodule(prefix))
        return 1;

    /* Fail */
    return 0;
}


/* search_for_exec_prefix requires that argv0_path be no more than
   MAXPATHLEN bytes long.
*/
static int
search_for_exec_prefix(wchar_t *argv0_path, wchar_t *home,
                       wchar_t *_exec_prefix, wchar_t *lib_python)
{
    size_t n;

    /* If PYTHONHOME is set, we believe it unconditionally */
    if (home) {
        wchar_t *delim;
        delim = wcschr(home, DELIM);
        if (delim)
            wcsncpy(exec_prefix, delim+1, MAXPATHLEN);
        else
            wcsncpy(exec_prefix, home, MAXPATHLEN);
        exec_prefix[MAXPATHLEN] = L'\0';
        joinpath(exec_prefix, lib_python);
        joinpath(exec_prefix, L"lib-dynload");
        return 1;
    }

    /* Check to see if argv[0] is in the build directory. "pybuilddir.txt"
       is written by setup.py and contains the relative path to the location
       of shared library modules. */
    wcsncpy(exec_prefix, argv0_path, MAXPATHLEN);
    exec_prefix[MAXPATHLEN] = L'\0';
    joinpath(exec_prefix, L"pybuilddir.txt");
    if (isfile(exec_prefix)) {
        FILE *f = _Py_wfopen(exec_prefix, L"rb");
        if (f == NULL)
            errno = 0;
        else {
            char buf[MAXPATHLEN+1];
            PyObject *decoded;
            wchar_t rel_builddir_path[MAXPATHLEN+1];
            n = fread(buf, 1, MAXPATHLEN, f);
            buf[n] = '\0';
            fclose(f);
            decoded = PyUnicode_DecodeUTF8(buf, n, "surrogateescape");
            if (decoded != NULL) {
                Py_ssize_t k;
                k = PyUnicode_AsWideChar(decoded,
                                         rel_builddir_path, MAXPATHLEN);
                Py_DECREF(decoded);
                if (k >= 0) {
                    rel_builddir_path[k] = L'\0';
                    wcsncpy(exec_prefix, argv0_path, MAXPATHLEN);
                    exec_prefix[MAXPATHLEN] = L'\0';
                    joinpath(exec_prefix, rel_builddir_path);
                    return -1;
                }
            }
        }
    }

    /* Search from argv0_path, until root is found */
    copy_absolute(exec_prefix, argv0_path, MAXPATHLEN+1);
    do {
        n = wcslen(exec_prefix);
        joinpath(exec_prefix, lib_python);
        joinpath(exec_prefix, L"lib-dynload");
        if (isdir(exec_prefix))
            return 1;
        exec_prefix[n] = L'\0';
        reduce(exec_prefix);
    } while (exec_prefix[0]);

    /* Look at configure's EXEC_PREFIX */
    wcsncpy(exec_prefix, _exec_prefix, MAXPATHLEN);
    exec_prefix[MAXPATHLEN] = L'\0';
    joinpath(exec_prefix, lib_python);
    joinpath(exec_prefix, L"lib-dynload");
    if (isdir(exec_prefix))
        return 1;

    /* Fail */
    return 0;
}

static void
calculate_path(void)
{
    extern wchar_t *Py_GetProgramName(void);
    wchar_t *pythonpath = PYTHONPATH;
    static const wchar_t delimiter[2] = {DELIM, '\0'};
    static const wchar_t separator[2] = {SEP, '\0'};
    //char *rtpypath = Py_GETENV("PYTHONPATH"); /* XXX use wide version on Windows */
    wchar_t *rtpypath = NULL;
    char *_path = getenv("path");
    wchar_t *path_buffer = NULL;
    wchar_t *path = NULL;
    wchar_t *prog = Py_GetProgramName();
    wchar_t argv0_path[MAXPATHLEN+1];
    wchar_t zip_path[MAXPATHLEN+1];
    wchar_t *buf;
    size_t bufsz;
    size_t prefixsz;
    wchar_t *defpath;

    if (_path) {
        path_buffer = Py_DecodeLocale(_path, NULL);
        path = path_buffer;
    }
/* ###########################################################################
      Determine path to the Python.efi binary.
      Produces progpath, argv0_path, and volume_name.
########################################################################### */

    /* If there is no slash in the argv0 path, then we have to
     * assume python is on the user's $PATH, since there's no
     * other way to find a directory to start the search from.  If
     * $PATH isn't exported, you lose.
     */
	if (wcspbrk(prog, L"/\\"))
	{
       wcsncpy(progpath, prog, MAXPATHLEN);
	}
    else if (path) {
      while (1) {
        wchar_t *delim = wcschr(path, DELIM);

        if (delim) {
			size_t len = delim - path;
			if (len > MAXPATHLEN)
				len = MAXPATHLEN;
			wcsncpy(progpath, path, len);
			*(progpath + len) = L'\0';
        }
        else
           wcsncpy(progpath, path, MAXPATHLEN);

        joinpath(progpath, prog);
        if (isxfile(progpath))
            break;

        if (!delim) {
            progpath[0] = L'\0';
            break;
        }
        path = delim + 1;
      }
    }
    else
        progpath[0] = L'\0';

    if ( (!is_absolute(progpath)) && (progpath[0] != '\0') )
            absolutize(progpath);

    wcsncpy(argv0_path, progpath, MAXPATHLEN);
    argv0_path[MAXPATHLEN] = L'\0';
    set_volume(volume_name, argv0_path);

    reduce(argv0_path);
    /* At this point, argv0_path is guaranteed to be less than
       MAXPATHLEN bytes long.
    */
/* ###########################################################################
      Build the FULL prefix string, including volume name.
      This is the full path to the platform independent libraries.
########################################################################### */

    wcsncpy(prefix, volume_name, MAXPATHLEN);
    joinpath(prefix, PREFIX);
    joinpath(prefix, lib_python);

/* ###########################################################################
      Build the FULL path to the zipped-up Python library.
########################################################################### */

    wcsncpy(zip_path, prefix, MAXPATHLEN);
    zip_path[MAXPATHLEN] = L'\0';
    reduce(zip_path);
    joinpath(zip_path, L"python00.zip");
    bufsz = wcslen(zip_path);   /* Replace "00" with version */
    zip_path[bufsz - 6] = VERSION[0];
    zip_path[bufsz - 5] = VERSION[1];
/* ###########################################################################
      Build the FULL path to dynamically loadable libraries.
########################################################################### */

    wcsncpy(exec_prefix, volume_name, MAXPATHLEN);    // "fs0:"
    joinpath(exec_prefix, EXEC_PREFIX);               // "fs0:/Efi/StdLib"
    joinpath(exec_prefix, lib_python);                // "fs0:/Efi/StdLib/lib/python.27"
    joinpath(exec_prefix, L"lib-dynload");             // "fs0:/Efi/StdLib/lib/python.27/lib-dynload"
/* ###########################################################################
      Build the module search path.
########################################################################### */

    /* Reduce prefix and exec_prefix to their essence,
     * e.g. /usr/local/lib/python1.5 is reduced to /usr/local.
     * If we're loading relative to the build directory,
     * return the compiled-in defaults instead.
     */
    reduce(prefix);
    reduce(prefix);
    /* The prefix is the root directory, but reduce() chopped
     * off the "/". */
    if (!prefix[0]) {
      wcscpy(prefix, volume_name);
    }
    bufsz = wcslen(prefix);
    if(prefix[bufsz-1] == L':') {    // if prefix consists solely of a volume_name
      prefix[bufsz] = SEP;          //    then append SEP indicating the root directory
      prefix[bufsz+1] = 0;          //    and ensure the new string is terminated
    }

    /* Calculate size of return buffer.
     */
    defpath = pythonpath;
    bufsz = 0;

    if (rtpypath)
        bufsz += wcslen(rtpypath) + 1;

    prefixsz = wcslen(prefix) + 1;
    while (1) {
        wchar_t *delim = wcschr(defpath, DELIM);

        if (is_absolute(defpath) == 0)
            /* Paths are relative to prefix */
            bufsz += prefixsz;

        if (delim)
            bufsz += delim - defpath + 1;
        else {
            bufsz += wcslen(defpath) + 1;
            break;
        }
        defpath = delim + 1;
    }

    bufsz += wcslen(zip_path) + 1;
    bufsz += wcslen(exec_prefix) + 1;

    /* This is the only malloc call in this file */
	buf = (wchar_t *)PyMem_Malloc(bufsz * 2);

    if (buf == NULL) {
        /* We can't exit, so print a warning and limp along */
        fprintf(stderr, "Not enough memory for dynamic PYTHONPATH.\n");
        fprintf(stderr, "Using default static PYTHONPATH.\n");
        module_search_path = PYTHONPATH;
    }
    else {
        /* Run-time value of $PYTHONPATH goes first */
        if (rtpypath) {
            wcscpy(buf, rtpypath);
            wcscat(buf, delimiter);
        }
        else
            buf[0] = L'\0';

        /* Next is the default zip path */
        wcscat(buf, zip_path);
        wcscat(buf, delimiter);
        /* Next goes merge of compile-time $PYTHONPATH with
         * dynamically located prefix.
         */
        defpath = pythonpath;
        while (1) {
            wchar_t *delim = wcschr(defpath, DELIM);

            if (is_absolute(defpath) != 1) {
                wcscat(buf, prefix);
                wcscat(buf, separator);
            }

            if (delim) {
                size_t len = delim - defpath + 1;
                size_t end = wcslen(buf) + len;
                wcsncat(buf, defpath, len);
                *(buf + end) = L'\0';
            }
            else {
                wcscat(buf, defpath);
                break;
            }
            defpath = delim + 1;
        }
        wcscat(buf, delimiter);
        /* Finally, on goes the directory for dynamic-load modules */
        wcscat(buf, exec_prefix);
        /* And publish the results */
        module_search_path = buf;
    }
        /*  At this point, exec_prefix is set to VOL:/Efi/StdLib/lib/python.27/dynalib.
            We want to get back to the root value, so we have to remove the final three
            segments to get VOL:/Efi/StdLib.  Because we don't know what VOL is, and
            EXEC_PREFIX is also indeterminate, we just remove the three final segments.
        */
        reduce(exec_prefix);
        reduce(exec_prefix);
        reduce(exec_prefix);
        if (!exec_prefix[0]) {
          wcscpy(exec_prefix, volume_name);
        }
        bufsz = wcslen(exec_prefix);
        if(exec_prefix[bufsz-1] == L':') {
          exec_prefix[bufsz] = SEP;
          exec_prefix[bufsz+1] = 0;
        }

#if 1
    if (Py_VerboseFlag) PySys_WriteStderr("%s[%d]: module_search_path = \"%s\"\n", __func__, __LINE__, module_search_path);
    if (Py_VerboseFlag) PySys_WriteStderr("%s[%d]: prefix             = \"%s\"\n", __func__, __LINE__, prefix);
    if (Py_VerboseFlag) PySys_WriteStderr("%s[%d]: exec_prefix        = \"%s\"\n", __func__, __LINE__, exec_prefix);
    if (Py_VerboseFlag) PySys_WriteStderr("%s[%d]: progpath           = \"%s\"\n", __func__, __LINE__, progpath);
#endif 

#if 0

    extern wchar_t *Py_GetProgramName(void);

    static const wchar_t delimiter[2] = {DELIM, '\0'};
    static const wchar_t separator[2] = {SEP, '\0'};
    char *_rtpypath = Py_GETENV("PYTHONPATH"); /* XXX use wide version on Windows */
    wchar_t *rtpypath = NULL;
    //wchar_t *home = Py_GetPythonHome();
    char *_path = getenv("PATH");
    wchar_t *path_buffer = NULL;
    wchar_t *path = NULL;
    wchar_t *prog = Py_GetProgramName();
    wchar_t argv0_path[MAXPATHLEN+1];
    wchar_t zip_path[MAXPATHLEN+1];
    wchar_t *buf;
    size_t bufsz;
    size_t prefixsz;
    wchar_t *defpath;
#ifdef WITH_NEXT_FRAMEWORK
    NSModule pythonModule;
    const char*    modPath;
#endif
#ifdef __APPLE__
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
    uint32_t nsexeclength = MAXPATHLEN;
#else
    unsigned long nsexeclength = MAXPATHLEN;
#endif
    char execpath[MAXPATHLEN+1];
#endif
    wchar_t *_pythonpath, *_prefix, *_exec_prefix;
    wchar_t *lib_python;

    _pythonpath = Py_DecodeLocale(PYTHONPATH, NULL);
    _prefix = Py_DecodeLocale(PREFIX, NULL);
    _exec_prefix = Py_DecodeLocale(EXEC_PREFIX, NULL);
    lib_python = Py_DecodeLocale("lib/python" VERSION, NULL);

    if (!_pythonpath || !_prefix || !_exec_prefix || !lib_python) {
        Py_FatalError(
            "Unable to decode path variables in getpath.c: "
            "memory error");
    }

    if (_path) {
        path_buffer = Py_DecodeLocale(_path, NULL);
        path = path_buffer;
    }

    /* If there is no slash in the argv0 path, then we have to
     * assume python is on the user's $PATH, since there's no
     * other way to find a directory to start the search from.  If
     * $PATH isn't exported, you lose.
     */
    if (wcschr(prog, SEP))
        wcsncpy(progpath, prog, MAXPATHLEN);
#ifdef __APPLE__
     /* On Mac OS X, if a script uses an interpreter of the form
      * "#!/opt/python2.3/bin/python", the kernel only passes "python"
      * as argv[0], which falls through to the $PATH search below.
      * If /opt/python2.3/bin isn't in your path, or is near the end,
      * this algorithm may incorrectly find /usr/bin/python. To work
      * around this, we can use _NSGetExecutablePath to get a better
      * hint of what the intended interpreter was, although this
      * will fail if a relative path was used. but in that case,
      * absolutize() should help us out below
      */
    else if(0 == _NSGetExecutablePath(execpath, &nsexeclength) && execpath[0] == SEP) {
        size_t r = mbstowcs(progpath, execpath, MAXPATHLEN+1);
        if (r == (size_t)-1 || r > MAXPATHLEN) {
            /* Could not convert execpath, or it's too long. */
            progpath[0] = L'\0';
        }
    }
#endif /* __APPLE__ */
    else if (path) {
        while (1) {
            wchar_t *delim = wcschr(path, DELIM);

            if (delim) {
                size_t len = delim - path;
                if (len > MAXPATHLEN)
                    len = MAXPATHLEN;
                wcsncpy(progpath, path, len);
                *(progpath + len) = L'\0';
            }
            else
                wcsncpy(progpath, path, MAXPATHLEN);

            joinpath(progpath, prog);
            if (isxfile(progpath))
                break;

            if (!delim) {
                progpath[0] = L'\0';
                break;
            }
            path = delim + 1;
        }
    }
    else
        progpath[0] = L'\0';
    PyMem_RawFree(path_buffer);
    if (progpath[0] != SEP && progpath[0] != L'\0')
        absolutize(progpath);
    wcsncpy(argv0_path, progpath, MAXPATHLEN);
    argv0_path[MAXPATHLEN] = L'\0';

#ifdef WITH_NEXT_FRAMEWORK
    /* On Mac OS X we have a special case if we're running from a framework.
    ** This is because the python home should be set relative to the library,
    ** which is in the framework, not relative to the executable, which may
    ** be outside of the framework. Except when we're in the build directory...
    */
    pythonModule = NSModuleForSymbol(NSLookupAndBindSymbol("_Py_Initialize"));
    /* Use dylib functions to find out where the framework was loaded from */
    modPath = NSLibraryNameForModule(pythonModule);
    if (modPath != NULL) {
        /* We're in a framework. */
        /* See if we might be in the build directory. The framework in the
        ** build directory is incomplete, it only has the .dylib and a few
        ** needed symlinks, it doesn't have the Lib directories and such.
        ** If we're running with the framework from the build directory we must
        ** be running the interpreter in the build directory, so we use the
        ** build-directory-specific logic to find Lib and such.
        */
        wchar_t* wbuf = Py_DecodeLocale(modPath, NULL);
        if (wbuf == NULL) {
            Py_FatalError("Cannot decode framework location");
        }

        wcsncpy(argv0_path, wbuf, MAXPATHLEN);
        reduce(argv0_path);
        joinpath(argv0_path, lib_python);
        joinpath(argv0_path, LANDMARK);
        if (!ismodule(argv0_path)) {
            /* We are in the build directory so use the name of the
               executable - we know that the absolute path is passed */
            wcsncpy(argv0_path, progpath, MAXPATHLEN);
        }
        else {
            /* Use the location of the library as the progpath */
            wcsncpy(argv0_path, wbuf, MAXPATHLEN);
        }
        PyMem_RawFree(wbuf);
    }
#endif

#if HAVE_READLINK
    {
        wchar_t tmpbuffer[MAXPATHLEN+1];
        int linklen = _Py_wreadlink(progpath, tmpbuffer, MAXPATHLEN);
        while (linklen != -1) {
            if (tmpbuffer[0] == SEP)
                /* tmpbuffer should never be longer than MAXPATHLEN,
                   but extra check does not hurt */
                wcsncpy(argv0_path, tmpbuffer, MAXPATHLEN);
            else {
                /* Interpret relative to progpath */
                reduce(argv0_path);
                joinpath(argv0_path, tmpbuffer);
            }
            linklen = _Py_wreadlink(argv0_path, tmpbuffer, MAXPATHLEN);
        }
    }
#endif /* HAVE_READLINK */

    reduce(argv0_path);
    /* At this point, argv0_path is guaranteed to be less than
       MAXPATHLEN bytes long.
    */

    /* Search for an environment configuration file, first in the
       executable's directory and then in the parent directory.
       If found, open it for use when searching for prefixes.
    */

    {
        wchar_t tmpbuffer[MAXPATHLEN+1];
        wchar_t *env_cfg = L"pyvenv.cfg";
        FILE * env_file = NULL;

        wcscpy(tmpbuffer, argv0_path);

        joinpath(tmpbuffer, env_cfg);
        env_file = _Py_wfopen(tmpbuffer, L"r");
        if (env_file == NULL) {
            errno = 0;
            reduce(tmpbuffer);
            reduce(tmpbuffer);
            joinpath(tmpbuffer, env_cfg);
            env_file = _Py_wfopen(tmpbuffer, L"r");
            if (env_file == NULL) {
                errno = 0;
            }
        }
        if (env_file != NULL) {
            /* Look for a 'home' variable and set argv0_path to it, if found */
            if (find_env_config_value(env_file, L"home", tmpbuffer)) {
                wcscpy(argv0_path, tmpbuffer);
            }
            fclose(env_file);
            env_file = NULL;
        }
    }
    printf("argv0_path = %s, home = %s, _prefix = %s, lib_python=%s",argv0_path, home, _prefix, lib_python); 

    pfound = search_for_prefix(argv0_path, home, _prefix, lib_python);
    if (!pfound) {
        if (!Py_FrozenFlag)
            fprintf(stderr,
                "Could not find platform independent libraries <prefix>\n");
        wcsncpy(prefix, _prefix, MAXPATHLEN);
        joinpath(prefix, lib_python);
    }
    else
        reduce(prefix);

    wcsncpy(zip_path, prefix, MAXPATHLEN);
    zip_path[MAXPATHLEN] = L'\0';
    if (pfound > 0) { /* Use the reduced prefix returned by Py_GetPrefix() */
        reduce(zip_path);
        reduce(zip_path);
    }
    else
        wcsncpy(zip_path, _prefix, MAXPATHLEN);
    joinpath(zip_path, L"lib/python36.zip");
    bufsz = wcslen(zip_path);   /* Replace "00" with version */
    zip_path[bufsz - 6] = VERSION[0];
    zip_path[bufsz - 5] = VERSION[2];

    efound = search_for_exec_prefix(argv0_path, home,
                                    _exec_prefix, lib_python);
    if (!efound) {
        if (!Py_FrozenFlag)
            fprintf(stderr,
                "Could not find platform dependent libraries <exec_prefix>\n");
        wcsncpy(exec_prefix, _exec_prefix, MAXPATHLEN);
        joinpath(exec_prefix, L"lib/lib-dynload");
    }
    /* If we found EXEC_PREFIX do *not* reduce it!  (Yet.) */

    if ((!pfound || !efound) && !Py_FrozenFlag)
        fprintf(stderr,
                "Consider setting $PYTHONHOME to <prefix>[:<exec_prefix>]\n");

    /* Calculate size of return buffer.
     */
    bufsz = 0;

    if (_rtpypath && _rtpypath[0] != '\0') {
        size_t rtpypath_len;
        rtpypath = Py_DecodeLocale(_rtpypath, &rtpypath_len);
        if (rtpypath != NULL)
            bufsz += rtpypath_len + 1;
    }

    defpath = _pythonpath;
    prefixsz = wcslen(prefix) + 1;
    while (1) {
        wchar_t *delim = wcschr(defpath, DELIM);

        if (defpath[0] != SEP)
            /* Paths are relative to prefix */
            bufsz += prefixsz;

        if (delim)
            bufsz += delim - defpath + 1;
        else {
            bufsz += wcslen(defpath) + 1;
            break;
        }
        defpath = delim + 1;
    }

    bufsz += wcslen(zip_path) + 1;
    bufsz += wcslen(exec_prefix) + 1;

    buf = PyMem_RawMalloc(bufsz * sizeof(wchar_t));
    if (buf == NULL) {
        Py_FatalError(
            "Not enough memory for dynamic PYTHONPATH");
    }

    /* Run-time value of $PYTHONPATH goes first */
    if (rtpypath) {
        wcscpy(buf, rtpypath);
        wcscat(buf, delimiter);
    }
    else
        buf[0] = '\0';

    /* Next is the default zip path */
    wcscat(buf, zip_path);
    wcscat(buf, delimiter);

    /* Next goes merge of compile-time $PYTHONPATH with
     * dynamically located prefix.
     */
    defpath = _pythonpath;
    while (1) {
        wchar_t *delim = wcschr(defpath, DELIM);

        if (defpath[0] != SEP) {
            wcscat(buf, prefix);
            if (prefixsz >= 2 && prefix[prefixsz - 2] != SEP &&
                defpath[0] != (delim ? DELIM : L'\0')) {  /* not empty */
                wcscat(buf, separator);
            }
        }

        if (delim) {
            size_t len = delim - defpath + 1;
            size_t end = wcslen(buf) + len;
            wcsncat(buf, defpath, len);
            *(buf + end) = '\0';
        }
        else {
            wcscat(buf, defpath);
            break;
        }
        defpath = delim + 1;
    }
    wcscat(buf, delimiter);

    /* Finally, on goes the directory for dynamic-load modules */
    wcscat(buf, exec_prefix);

    /* And publish the results */
    module_search_path = buf;

    /* Reduce prefix and exec_prefix to their essence,
     * e.g. /usr/local/lib/python1.5 is reduced to /usr/local.
     * If we're loading relative to the build directory,
     * return the compiled-in defaults instead.
     */
    if (pfound > 0) {
        reduce(prefix);
        reduce(prefix);
        /* The prefix is the root directory, but reduce() chopped
         * off the "/". */
        if (!prefix[0])
                wcscpy(prefix, separator);
    }
    else
        wcsncpy(prefix, _prefix, MAXPATHLEN);

    if (efound > 0) {
        reduce(exec_prefix);
        reduce(exec_prefix);
        reduce(exec_prefix);
        if (!exec_prefix[0])
                wcscpy(exec_prefix, separator);
    }
    else
        wcsncpy(exec_prefix, _exec_prefix, MAXPATHLEN);

    PyMem_RawFree(_pythonpath);
    PyMem_RawFree(_prefix);
    PyMem_RawFree(_exec_prefix);
    PyMem_RawFree(lib_python);
    PyMem_RawFree(rtpypath);
#endif 
}


/* External interface */
void
Py_SetPath(const wchar_t *path)
{
    if (module_search_path != NULL) {
        PyMem_RawFree(module_search_path);
        module_search_path = NULL;
    }
    if (path != NULL) {
        extern wchar_t *Py_GetProgramName(void);
        wchar_t *prog = Py_GetProgramName();
        wcsncpy(progpath, prog, MAXPATHLEN);
        exec_prefix[0] = prefix[0] = L'\0';
        module_search_path = PyMem_RawMalloc((wcslen(path) + 1) * sizeof(wchar_t));
        if (module_search_path != NULL)
            wcscpy(module_search_path, path);
    }
}

wchar_t *
Py_GetPath(void)
{
    if (!module_search_path)
        calculate_path();
    return module_search_path;
}

wchar_t *
Py_GetPrefix(void)
{
    if (!module_search_path)
        calculate_path();
    return prefix;
}

wchar_t *
Py_GetExecPrefix(void)
{
    if (!module_search_path)
        calculate_path();
    return exec_prefix;
}

wchar_t *
Py_GetProgramFullPath(void)
{
    if (!module_search_path)
        calculate_path();
    return progpath;
}


#ifdef __cplusplus
}
#endif
