@echo off
setlocal EnableExtensions

REM Stage Python 3.12.13 UEFI runtime tree (3.6.8-style PREFIX under fsN:\EFI\StdLib)
REM
REM   <OutFolder>\EFI\Tools\Python.efi   (build still produces Python312.efi)
REM   <OutFolder>\EFI\StdLib\lib\python3.12\
REM   <OutFolder>\EFI\StdLib\etc\

set TOOL_CHAIN_TAG=%1
set TARGET=%2
set ARCH=%3
set OUT_FOLDER=%4

if "%TOOL_CHAIN_TAG%"=="" goto usage
if "%TARGET%"=="" goto usage
if "%ARCH%"=="" goto usage
if "%OUT_FOLDER%"=="" goto usage
goto continue

:usage
echo.
echo Create Python 3.12 EFI package (3.6.8-style StdLib layout).
echo.
echo Usage: %0 ^<ToolChain^> ^<Target^> ^<Architecture^> ^<OutFolder^>
echo   ToolChain     e.g. GCC, VS2019, VS2022
echo   Target        e.g. NOOPT, DEBUG, RELEASE
echo   Architecture  e.g. X64
echo   OutFolder     destination directory for the EFI\ tree
echo.
exit /b 1

:continue
if "%WORKSPACE%"=="" (
  echo WORKSPACE is not set.
  exit /b 1
)

if "%EDK2_LIBC_PATH%"=="" (
  echo Warning: EDK2_LIBC_PATH not set; using WORKSPACE.
  set "EDK2_LIBC_PATH=%WORKSPACE%"
)

set "PYTHON_SRC=%EDK2_LIBC_PATH%\AppPkg\Applications\Python\Python-3.12.13"
set "BUILD_DIR=%WORKSPACE%\Build\AppPkg\%TARGET%_%TOOL_CHAIN_TAG%\%ARCH%"
set "PYTHON_BIN=%BUILD_DIR%\Python312.efi"

REM Prefer FULL DEBUG, then OUTPUT, then any Python312.efi under BUILD_DIR
REM (EDK embeds the edk2-libc package folder name; do not hard-code it).
if exist "%PYTHON_BIN%" goto :have_python_bin
for /r "%BUILD_DIR%" %%F in (Python312.efi) do (
  echo %%F | findstr /i "\\Python312\\DEBUG\\Python312.efi" >nul
  if not errorlevel 1 set "PYTHON_BIN=%%F" & goto :have_python_bin
)
for /r "%BUILD_DIR%" %%F in (Python312.efi) do (
  echo %%F | findstr /i "\\Python312\\OUTPUT\\Python312.efi" >nul
  if not errorlevel 1 set "PYTHON_BIN=%%F" & goto :have_python_bin
)
for /r "%BUILD_DIR%" %%F in (Python312.efi) do (
  set "PYTHON_BIN=%%F" & goto :have_python_bin
)
goto error
:have_python_bin
if not exist "%PYTHON_BIN%" goto error

if not exist "%PYTHON_SRC%\Lib\" (
  echo Python Lib\ not found at %PYTHON_SRC%\Lib
  exit /b 1
)

if exist "%PYTHON_SRC%\srcprep.py" (
  where python >nul 2>&1
  if not errorlevel 1 (
    python "%PYTHON_SRC%\srcprep.py"
  ) else (
    where py >nul 2>&1
    if not errorlevel 1 (
      py -3.10 "%PYTHON_SRC%\srcprep.py"
    ) else (
      echo Warning: host python not found; run srcprep.py manually if PyMod Lib changed.
    )
  )
)

mkdir "%OUT_FOLDER%\EFI\Tools" 2>nul
mkdir "%OUT_FOLDER%\EFI\StdLib\lib\python3.12" 2>nul
mkdir "%OUT_FOLDER%\EFI\StdLib\lib\python3.12\lib-dynload" 2>nul
mkdir "%OUT_FOLDER%\EFI\StdLib\etc" 2>nul

copy /Y "%PYTHON_BIN%" "%OUT_FOLDER%\EFI\Tools\Python.efi" >nul
xcopy "%PYTHON_SRC%\Lib\*" "%OUT_FOLDER%\EFI\StdLib\lib\python3.12\" /Y /S /I /Q >nul
if exist "%PYTHON_SRC%\PyMod-3.12.13\Lib\" (
  xcopy "%PYTHON_SRC%\PyMod-3.12.13\Lib\*" "%OUT_FOLDER%\EFI\StdLib\lib\python3.12\" /Y /S /I /Q >nul
)
set "READLINE_VENDOR=%PYTHON_SRC%\PyMod-3.12.13\Modules\readline"
if exist "%READLINE_VENDOR%\pyreadline\" (
  if exist "%READLINE_VENDOR%\readline.py" (
    copy /Y "%READLINE_VENDOR%\readline.py" "%OUT_FOLDER%\EFI\StdLib\lib\python3.12\readline.py" >nul
    xcopy "%READLINE_VENDOR%\pyreadline\*" "%OUT_FOLDER%\EFI\StdLib\lib\python3.12\pyreadline\" /Y /S /I /Q >nul
    echo Staged pyreadline from %READLINE_VENDOR%
  )
) else (
  echo Warning: %READLINE_VENDOR% missing; import readline will fail.
)
if exist "%EDK2_LIBC_PATH%\StdLib\Efi\StdLib\etc\" (
  xcopy "%EDK2_LIBC_PATH%\StdLib\Efi\StdLib\etc\*" "%OUT_FOLDER%\EFI\StdLib\etc\" /Y /S /I /Q >nul
)

echo.
echo Python 3.12 EFI package ready at %OUT_FOLDER%\EFI\
echo   Tools\Python.efi
echo   StdLib\lib\python3.12\
echo   StdLib\etc\
echo.
echo UEFI Shell: fs0: then cd EFI\Tools then Python.efi
exit /b 0

:error
echo Failed to create Python EFI package.
echo Python312.efi not found under Build\AppPkg\%TARGET%_%TOOL_CHAIN_TAG%\%ARCH%\
echo Build with: build -a %ARCH% -b %TARGET% -t %TOOL_CHAIN_TAG% -p AppPkg\AppPkg.dsc -D BUILD_PYTHON312
exit /b 1
