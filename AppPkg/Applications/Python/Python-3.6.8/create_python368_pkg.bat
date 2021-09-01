@echo off

set TOOL_CHAIN_TAG=%1
set TARGET=%2
set OUT_FOLDER=%3
if "%TOOL_CHAIN_TAG%"=="" goto usage
if "%TARGET%"=="" goto usage
if "%OUT_FOLDER%"=="" goto usage
goto continue

:usage
echo.
echo.
echo.
echo Creates Python EFI Package. 
echo.
echo "Usage: %0 <ToolChain> <Target> <OutFolder>"
echo.
echo    ToolChain  = one of VS2013x86, VS2015x86, VS2017, VS2019
echo    Target     = one of RELEASE, DEBUG
echo    OutFolder  = Target folder where package needs to create
echo.
echo.
echo.

goto :eof

:continue
cd ..\..\..\..\
IF NOT EXIST Build\AppPkg\%TARGET%_%TOOL_CHAIN_TAG%\X64\Python368.efi goto error
mkdir %OUT_FOLDER%\EFI\Tools
xcopy Build\AppPkg\%TARGET%_%TOOL_CHAIN_TAG%\X64\Python368.efi %OUT_FOLDER%\EFI\Tools\ /y
mkdir %OUT_FOLDER%\EFI\StdLib\lib\python36.8
mkdir %OUT_FOLDER%\EFI\StdLib\etc
xcopy AppPkg\Applications\Python\Python-3.6.8\Lib\*    %OUT_FOLDER%\EFI\StdLib\lib\python36.8\      /Y /S /I
xcopy StdLib\Efi\StdLib\etc\*    %OUT_FOLDER%\EFI\StdLib\etc\      /Y /S /I
goto all_done

:error
echo Failed to Create Python 3.6.8 Package, Python368.efi is not available on build location Build\AppPkg\%TARGET%_%TOOL_CHAIN_TAG%\X64\


:all_done
exit /b %ec%




