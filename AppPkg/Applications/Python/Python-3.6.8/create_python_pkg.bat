@echo off

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
echo Batch Script to create Python EFI Package.
echo.
echo Invalid command line arguments passed, please see the below usage instructions
echo.
echo "Usage: %0 <ToolChain> <Target> <Architecture> <OutFolder>"
echo.
echo    ToolChain     = one of VS2013x86, VS2015x86, VS2017, VS2019
echo    Target        = one of RELEASE, DEBUG
echo    Architecture  = one of IA32, X64
echo    OutFolder     = Output directory for creating the package
echo.

goto :eof

:continue
cd ..\..\..\..\
if not exist Build\AppPkg\%TARGET%_%TOOL_CHAIN_TAG%\%ARCH%\Python.efi (
    goto error
)

if not exist %OUT_FOLDER%\EFI\Tools (
   mkdir %OUT_FOLDER%\EFI\Tools
)
xcopy Build\AppPkg\%TARGET%_%TOOL_CHAIN_TAG%\%ARCH%\Python.efi %OUT_FOLDER%\EFI\Tools\ /y

if not exist %OUT_FOLDER%\EFI\StdLib\lib\python36.8 (
    mkdir %OUT_FOLDER%\EFI\StdLib\lib\python36.8
)
if not exist %OUT_FOLDER%\EFI\StdLib\etc (
   mkdir %OUT_FOLDER%\EFI\StdLib\etc
)
xcopy AppPkg\Applications\Python\Python-3.6.8\Lib\*  %OUT_FOLDER%\EFI\StdLib\lib\python36.8\    /Y /S /I
xcopy StdLib\Efi\StdLib\etc\*  %OUT_FOLDER%\EFI\StdLib\etc\  /Y /S /I
echo.

if not x%OUT_FOLDER::=%==x%OUT_FOLDER% (
    echo Python EFI package available at %OUT_FOLDER%
) else (
    echo Python EFI package available at %CD%\%OUT_FOLDER%
)
goto all_done

:error
echo Failed to Create Python EFI Package
echo Python.efi is not available at Build\AppPkg\%TARGET%_%TOOL_CHAIN_TAG%\%ARCH%\
echo Follow the instructions in Py368ReadMe.txt to build Python interpreter
echo Then use this script to create a Python EFI package

:all_done
exit /b %ERRORLEVEL%
