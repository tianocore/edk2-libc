                                EDK II Python
                                   ReadMe
                                Version 3.6.8
                                 Release 1.00
                               01 September 2021


1. OVERVIEW
===========
This document is devoted to general information on building and setup of the
Python environment for UEFI, the invocation of the interpreter, and things
that make working with Python easier.

It is assumed that you already have UDK2018 or later, or a current snapshot of
the EDK II sources from www.tianocore.org (https://github.com/tianocore/edk2), 
and that you can successfully build packages within that distribution.

2. Release Notes
================
  1)  All C extension modules must be statically linked (built in)
  2)  The site and os modules must exist as discrete files in ...\lib\python36.8
  3)  User-specific configurations are not supported.
  4)  Environment variables are not supported.

3. Getting and Building Python
======================================================
  3.1 Getting Python
  ==================
  This file describes the UEFI port of version 3.6.8 of the CPython distribution.
  For development ease, a subset of the Python 3.6.8 distribution has been
  included as part of the AppPkg/Applications/Python/Python-3.6.8 source tree.
  If this is sufficient, you may skip to section 3.2, Building Python.

  If a full distribution is desired, it can be merged into the Python-3.6.8
  source tree.  Directory AppPkg/Applications/Python/Python-3.6.8 corresponds
  to the root directory of the CPython 3.6.8 distribution.  The full
  CPython 3.6.8 source code may be downloaded from
  http://www.python.org/ftp/python/3.6.8/.

  A.  Within your EDK II development tree, extract the Python distribution into
    AppPkg/Applications/Python/Python-3.6.8.  This should merge the additional
    files into the source tree.  It will also create the following directories:
        Demo      Doc         Grammar     Mac       Misc
        PC        PCbuild     RISCOS      Tools

    The greatest change will be within the Python-3.6.8/Lib directory where
    many more packages and modules will be added.  These additional components
    may not have been ported to EDK II yet.

  3.2 Building Python
  ===================
  A.  From the AppPkg/Applications/Python/Python-3.6.8 directory, execute the
    srcprep.py script to copy the header files from within the
    PyMod-3.6.8 sub-tree into their corresponding directories within the
    distribution.  This step only needs to be performed prior to the first
    build of Python, or if one of the header files within the PyMod tree has been
    modified.

  B.  Edit PyMod-3.6.8\Modules\config.c to enable the built-in modules you need.
    By default, it is configured for the minimally required set of modules.
      Mandatory Built-in Modules:
        edk2      errno       imp         marshal

      Additional built-in modules which are required to use the help()
      functionality provided by PyDoc, are:
        _codecs     _collections    _functools    _random
        _sre        _struct         _weakref      binascii
        gc          itertools       math          _operator
        time

  C.  Set the PACKAGES_PATH and EDK2_LIBC_PATH environment variables to
    the right values. PACKAGES_PATH should be set to the folder path of
    edk2 and edk2-libc folders. EDK2_LIBC_PATH should be set to the
    folder path of edk2-libc.
    Use the below provided commands as reference to set the environment
    variables to the corresponding values

                set PACKAGES_PATH=<path_to_edk2>;<path_to_edk2_libc>;
        set EDK2_LIBC_PATH=<path_to_edk2_libc>

    where,
        <path_to_edk2> should be replaced with the absolute path to
                       edk2 folder on your development system.

        <path_to_edk2_libc> should be replaced with the absolute path
                       to edk2-libc folder on your development system.

  D.  Build AppPkg using the standard "build" command:
    For example, to build Python for an X64 CPU architecture:
                    build -a X64 -p AppPkg\AppPkg.dsc -D BUILD_PYTHON368

4. Python-related paths and files
=================================
Python depends upon the existence of several directories and files on the
target system.

  \EFI                              Root of the UEFI system area.
   |- \Tools                        Location of the Python.efi executable.
   |- \Boot                         UEFI specified Boot directory.
   |- \StdLib                       Root of the Standard Libraries sub-tree.
       |- \etc                      Configuration files used by libraries.
       |- \lib                      Root of the libraries tree.
           |- \python36.8           Directory containing the Python library
               |                    modules.
               |- \lib-dynload      Dynamically loadable Python extensions. 
               |                    Not supported currently.                     
               |- \site-packages    Site-specific packages and modules.


5. Installing Python
====================
These directories, on the target system, are populated from the development
system as follows:

  * \Efi\Tools receives a copy of Build/AppPkg/RELEASE_VS2017/X64/Python.efi.
                                               ^^^^^^^^^^^^^^^^
    Modify the host path to match your build type and compiler.

  * The \Efi\StdLib\etc directory is populated from the StdLib/Efi/StdLib/etc
    source directory.

  * Directory \Efi\StdLib\lib\python36.8 is populated with packages and modules
    from the AppPkg/Applications/Python/Python-3.6.8/Lib directory.
    The recommended minimum set of modules (.py, .pyc, and/or .pyo):
        os      stat      ntpath      warnings      traceback
        site    types     linecache   genericpath

  * Python C Extension Modules built as dynamically loadable extensions go into
    the \Efi\StdLib\lib\python36.8\lib-dynload directory.  This functionality is not
    yet implemented.

  A script, create_python_pkg.bat , is provided which facilitates the population
  of the target EFI package.  Execute this script from within the
  AppPkg/Applications/Python/Python-3.6.8 directory, providing the Tool Chain, Target
  Build and destination directory which is the path to the destination directory.
  Ensure that EDK2_LIBC_PATH environment variable has been set to edk2-libc folder path.
  The appropriate contents of the AppPkg/Applications/Python/Python-3.6.8/Lib and
  Python.efi Application from Build/AppPkg/RELEASE_VS2017/X64/ will be
                                              ^^^^^^^^^^^^^^   
  copied into the specified destination directory.

  Replace "RELEASE_VS2017", in the source path, with values appropriate for your tool chain.


6. Example: Enabling socket support
===================================
  1. Uncomment the statement // {"_socket", init_socket}, in PyMod-3.6.8\Modules\config.c
  2. Uncomment BsdSocketLib and EfiSocketLib LibraryClasses in Python368.inf
  3. Uncomment the statement #Modules/socketmodule.c in Python368.inf
  4. Build Python interpreter application using below command
          build -a X64 -p AppPkg\AppPkg.dsc -D BUILD_PYTHON368
  5. Copy Build\AppPkg\RELEASE_VS2017\X64\Python.efi to \Efi\Tools on your
     target system. Replace "RELEASE_VS2017", in the source path, with
     values appropriate for your tool chain.

7. Running Python
=================
  Python must currently be run from an EFI FAT-32 partition, or volume, under
  the UEFI Shell.  At the Shell prompt enter the desired volume name, followed
  by a colon ':', then press Enter.  Python can then be executed by typing its
  name, followed by any desired options and arguments.

  EXAMPLE:
      Shell> fs0:
      FS0:\> python
      Python 3.6.8 (default, Jun 24 2015, 17:38:32) [C] on uefi
      Type "help", "copyright", "credits" or "license" for more information.
      >>> exit()
      FS0:\>


8. Supported C Modules
======================
    Module Name               C File(s)
  ===============       =============================================
  _ast                  Python/Python-ast.c
  _codecs               Modules/_codecsmodule.c
  _collections          Modules/_collectionsmodule.c
  _csv                  Modules/_csv.c
  _functools            Modules/_functoolsmodule.c
  _io                   Modules/_io/_iomodule.c
  _json                 Modules/_json.c
  _md5                  Modules/md5module.c
  _multibytecodec       Modules/cjkcodecs/multibytecodec.c
  _random               Modules/_randommodule.c
  _sha1                 Modules/sha1module.c
  _sha256               Modules/sha256module.c
  _sha512               Modules/sha512module.c
  _sre                  Modules/_sre.c
  _struct               Modules/_struct.c
  _symtable             Modules/symtablemodule.c
  _weakref              Modules/_weakref.c
  array                 Modules/arraymodule.c
  atexit                Modules/atexitmodule.c
  binascii              Modules/binascii.c
  cmath                 Modules/cmathmodule.c
  datetime              Modules/_datetimemodule.c
  edk2                  Modules/PyMod-3.6.8/edk2module.c
  errno                 Modules/errnomodule.c
  gc                    Modules/gcmodule.c
  imp                   Python/import.c
  itertools             Modules/itertoolsmodule.c
  marshal               Python/marshal.c
  _operator             Modules/_operator.c
  parser                Modules/parsermodule.c
  select                Modules/selectmodule.c
  signal                Modules/signalmodule.c
  time                  Modules/timemodule.c
  zlib                  Modules/zlibmodule.c


9. Tested Python Library Modules
================================
This is a partial list of the packages and modules of the Python Standard
Library that have been tested or used in some manner.

  encodings               genericpath.py            site.py
  importlib               getopt.py                 socket.py
  json                    hashlib.py                sre.py
  pydoc_data              heapq.py                  sre_compile.py
  xml                     inspect.py                sre_constants.py
  abc.py                  io.py                     sre_parse.py
  argparse.py             keyword.py                stat.py       
  ast.py                  linecache.py              string.py  
  atexit.py               locale.py                 struct.py
  binhex.py               modulefinder.py           textwrap.py
  bisect.py               ntpath.py                 token.py       
  calendar.py             numbers.py                tokenize.py
  cmd.py                  optparse.py               traceback.py 
  codecs.py               os.py                     types.py
  collections.py          platform.py               warnings.py
  copy.py                 posixpath.py              weakref.py    
  csv.py                  pydoc.py                  zipfile.py
  fileinput.py            random.py               
  formatter.py            re.py                        
  functools.py            runpy.py
# # #
