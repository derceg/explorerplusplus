@echo off
setlocal enabledelayedexpansion

:: Explorer++ Command Line Build Script
:: This script builds Explorer++ using MSBuild.
:: It should be run from a Visual Studio Developer Command Prompt.

set CONFIGURATION=Release
set PLATFORM=x64

if "%1" NEQ "" set CONFIGURATION=%1
if "%2" NEQ "" set PLATFORM=%2

echo ============================================================
echo Building Explorer++
echo Configuration: %CONFIGURATION%
echo Platform:      %PLATFORM%
echo ============================================================

:: 1. Initialize vcpkg if not already done
if not exist "Explorer++\ThirdParty\vcpkg\vcpkg.exe" (
    echo Bootstrapping vcpkg...
    call "Explorer++\ThirdParty\vcpkg\bootstrap-vcpkg.bat"
    if %ERRORLEVEL% NEQ 0 (
        echo Error: vcpkg bootstrap failed.
        exit /b %ERRORLEVEL%
    )
)

:: 2. Build using MSBuild
:: We assume msbuild is in the PATH (standard for Developer Command Prompt)
where msbuild >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: msbuild not found. Please run this script from a
    echo Visual Studio Developer Command Prompt.
    exit /b 1
)

msbuild "Explorer++\Explorer++.sln" /p:Configuration=%CONFIGURATION% /p:Platform=%PLATFORM% /m

if %ERRORLEVEL% NEQ 0 (
    echo ============================================================
    echo Build FAILED!
    echo ============================================================
    exit /b %ERRORLEVEL%
)

echo ============================================================
echo Build Succeeded!
echo Output can be found in the Explorer++/Explorer++/bin directory (usually).
echo ============================================================

endlocal
