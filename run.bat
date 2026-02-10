@echo off
REM ============================================================================
REM Vulkan101 Run Script for Windows
REM ============================================================================

setlocal enabledelayedexpansion

echo.
echo ============================================================================
echo Vulkan101 Run Script
echo ============================================================================
echo.

REM Parse command line arguments
set BUILD_TYPE=Release

:parse_args
if "%~1"=="" goto :end_parse
if /i "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if /i "%~1"=="--release" (
    set BUILD_TYPE=Release
    shift
    goto :parse_args
)
shift
goto :parse_args
:end_parse

set EXECUTABLE=build\bin\%BUILD_TYPE%\Vulkan101.exe

REM Check if executable exists
if not exist "%EXECUTABLE%" (
    echo [ERROR] Executable not found: %EXECUTABLE%
    echo.
    echo Please build the project first:
    echo   build.bat
    echo.
    echo Or build in the correct configuration:
    echo   build.bat --%BUILD_TYPE%
    echo.
    exit /b 1
)

REM Check if assets are in place
if not exist "build\bin\models" (
    echo [WARNING] Models directory not found in build output
    echo Assets may not have been copied correctly
)

if not exist "build\bin\textures" (
    echo [WARNING] Textures directory not found in build output
    echo Assets may not have been copied correctly
)

if not exist "build\bin\shaders" (
    echo [WARNING] Shaders directory not found in build output
    echo Shaders may not have been compiled or copied
)

echo [INFO] Running Vulkan101 (%BUILD_TYPE% build)...
echo.

REM Change to the bin directory so it can find assets (shaders, models, textures)
cd build\bin

REM Run the application from the configuration subdirectory
%BUILD_TYPE%\Vulkan101.exe

REM Capture exit code
set EXIT_CODE=%ERRORLEVEL%

REM Return to original directory
cd ..\..

echo.
if %EXIT_CODE% EQU 0 (
    echo [INFO] Application exited successfully
) else (
    echo [ERROR] Application exited with code: %EXIT_CODE%
)
echo.

exit /b %EXIT_CODE%
