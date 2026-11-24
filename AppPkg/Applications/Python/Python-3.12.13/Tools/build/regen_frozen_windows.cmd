@echo off
setlocal EnableExtensions
REM Regenerate PyMod-3.12.13/Python/frozen_modules/*.h, deepfreeze.c,
REM and UEFI fork global-string headers (generate_global_objects + latin1 fix).
REM
REM Host: Python 3.12.x (e.g. 3.12.10) against this 3.12.13 source tree.
REM See AppPkg/Applications/Python/Python-3.12.13/Python312_Windows_VS2022_Build_Guide.md section 6.

cd /d "%~dp0..\.."
if errorlevel 1 (
    echo Failed to cd to Python-3.12.13 root
    exit /b 1
)

if not defined HOSTPY set "HOSTPY=py -3.12"
set "FM=Programs\_freeze_module.py"
set "OUT=PyMod-3.12.13\Python\frozen_modules"

echo === Host Python ===
%HOSTPY% -c "import sys, importlib._bootstrap_external as b; assert sys.version_info[:2]==(3,12), sys.version; print(sys.version); print('magic', b._RAW_MAGIC_NUMBER)"
if errorlevel 1 (
    echo Install Python 3.12.x and ensure py -3.12 works, or set HOSTPY=path\to\python.exe
    exit /b 1
)

echo === frozen_modules *.h ===
call :freeze importlib._bootstrap Lib\importlib\_bootstrap.py %OUT%\importlib._bootstrap.h || exit /b 1
call :freeze importlib._bootstrap_external Lib\importlib\_bootstrap_external.py %OUT%\importlib._bootstrap_external.h || exit /b 1
call :freeze zipimport Lib\zipimport.py %OUT%\zipimport.h || exit /b 1
call :freeze getpath Modules\getpath.py %OUT%\getpath.h || exit /b 1
call :freeze abc Lib\abc.py %OUT%\abc.h || exit /b 1
call :freeze codecs Lib\codecs.py %OUT%\codecs.h || exit /b 1
call :freeze io Lib\io.py %OUT%\io.h || exit /b 1
call :freeze _collections_abc Lib\_collections_abc.py %OUT%\_collections_abc.h || exit /b 1
call :freeze _sitebuiltins Lib\_sitebuiltins.py %OUT%\_sitebuiltins.h || exit /b 1
call :freeze genericpath Lib\genericpath.py %OUT%\genericpath.h || exit /b 1
call :freeze ntpath Lib\ntpath.py %OUT%\ntpath.h || exit /b 1
call :freeze posixpath Lib\posixpath.py %OUT%\posixpath.h || exit /b 1
call :freeze os Lib\os.py %OUT%\os.h || exit /b 1
call :freeze site Lib\site.py %OUT%\site.h || exit /b 1
call :freeze stat Lib\stat.py %OUT%\stat.h || exit /b 1
call :freeze importlib.util Lib\importlib\util.py %OUT%\importlib.util.h || exit /b 1
call :freeze importlib.machinery Lib\importlib\machinery.py %OUT%\importlib.machinery.h || exit /b 1
call :freeze runpy Lib\runpy.py %OUT%\runpy.h || exit /b 1
call :freeze __hello__ Lib\__hello__.py %OUT%\__hello__.h || exit /b 1
call :freeze __phello__ Lib\__phello__\__init__.py %OUT%\__phello__.h || exit /b 1
call :freeze __phello__.ham Lib\__phello__\ham\__init__.py %OUT%\__phello__.ham.h || exit /b 1
call :freeze __phello__.ham.eggs Lib\__phello__\ham\eggs.py %OUT%\__phello__.ham.eggs.h || exit /b 1
call :freeze __phello__.spam Lib\__phello__\spam.py %OUT%\__phello__.spam.h || exit /b 1
call :freeze frozen_only Tools\freeze\flag.py %OUT%\frozen_only.h || exit /b 1

echo === deepfreeze.c ===
if not exist PyMod-3.12.13\Python\deepfreeze mkdir PyMod-3.12.13\Python\deepfreeze
%HOSTPY% PyMod-3.12.13\Tools\build\deepfreeze.py ^
  PyMod-3.12.13/Python/frozen_modules/importlib._bootstrap.h:importlib._bootstrap ^
  PyMod-3.12.13/Python/frozen_modules/importlib._bootstrap_external.h:importlib._bootstrap_external ^
  PyMod-3.12.13/Python/frozen_modules/zipimport.h:zipimport ^
  PyMod-3.12.13/Python/frozen_modules/abc.h:abc ^
  PyMod-3.12.13/Python/frozen_modules/codecs.h:codecs ^
  PyMod-3.12.13/Python/frozen_modules/io.h:io ^
  PyMod-3.12.13/Python/frozen_modules/_collections_abc.h:_collections_abc ^
  PyMod-3.12.13/Python/frozen_modules/_sitebuiltins.h:_sitebuiltins ^
  PyMod-3.12.13/Python/frozen_modules/genericpath.h:genericpath ^
  PyMod-3.12.13/Python/frozen_modules/ntpath.h:ntpath ^
  PyMod-3.12.13/Python/frozen_modules/posixpath.h:posixpath ^
  PyMod-3.12.13/Python/frozen_modules/os.h:os ^
  PyMod-3.12.13/Python/frozen_modules/site.h:site ^
  PyMod-3.12.13/Python/frozen_modules/stat.h:stat ^
  PyMod-3.12.13/Python/frozen_modules/importlib.util.h:importlib.util ^
  PyMod-3.12.13/Python/frozen_modules/importlib.machinery.h:importlib.machinery ^
  PyMod-3.12.13/Python/frozen_modules/runpy.h:runpy ^
  PyMod-3.12.13/Python/frozen_modules/__hello__.h:__hello__ ^
  PyMod-3.12.13/Python/frozen_modules/__phello__.h:__phello__ ^
  PyMod-3.12.13/Python/frozen_modules/__phello__.ham.h:__phello__.ham ^
  PyMod-3.12.13/Python/frozen_modules/__phello__.ham.eggs.h:__phello__.ham.eggs ^
  PyMod-3.12.13/Python/frozen_modules/__phello__.spam.h:__phello__.spam ^
  PyMod-3.12.13/Python/frozen_modules/frozen_only.h:frozen_only ^
  -o PyMod-3.12.13\Python\deepfreeze\deepfreeze.c
if errorlevel 1 exit /b 1

echo === fix_deepfreeze_statically_allocated.py ===
%HOSTPY% PyMod-3.12.13\Tools\build\fix_deepfreeze_statically_allocated.py
if errorlevel 1 exit /b 1

echo === generate_global_objects.py ===
%HOSTPY% PyMod-3.12.13\Tools\build\generate_global_objects.py
if errorlevel 1 exit /b 1

echo === fix_deepfreeze_latin1.py ===
%HOSTPY% PyMod-3.12.13\Tools\build\fix_deepfreeze_latin1.py
if errorlevel 1 exit /b 1

findstr /c:".statically_allocated = 1," PyMod-3.12.13\Python\deepfreeze\deepfreeze.c >nul
if errorlevel 1 (
    echo WARNING: deepfreeze.c has no .statically_allocated = 1 markers — check deepfreeze.py / host Python
    exit /b 1
)

echo === OK: frozen regen complete ===
exit /b 0

:freeze
%HOSTPY% %FM% %1 %2 %3
if errorlevel 1 (
    echo freeze failed: %1
    exit /b 1
)
exit /b 0
