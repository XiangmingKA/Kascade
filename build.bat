@echo off
REM ============================================================================
REM Vulkan101 Build Script for Windows
REM ============================================================================

setlocal enabledelayedexpansion

echo.
echo ============================================================================
echo Vulkan101 Build Script
echo ============================================================================
echo.

REM Check if CMake is available
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake not found! Please install CMake and add it to PATH.
    echo Download from: https://cmake.org/download/
    exit /b 1
)

REM Check CMake version
for /f "tokens=3" %%i in ('cmake --version ^| findstr /R "version"') do set CMAKE_VERSION=%%i
echo [INFO] CMake version: %CMAKE_VERSION%

REM Parse command line arguments
set BUILD_TYPE=Release
set CLEAN_BUILD=0
set GENERATOR="Visual Studio 17 2022"
set PLATFORM=x64

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
if /i "%~1"=="--clean" (
    set CLEAN_BUILD=1
    shift
    goto :parse_args
)
if /i "%~1"=="--vs2019" (
    set GENERATOR="Visual Studio 16 2019"
    shift
    goto :parse_args
)
if /i "%~1"=="--vs2022" (
    set GENERATOR="Visual Studio 17 2022"
    shift
    goto :parse_args
)
if /i "%~1"=="--help" (
    goto :show_help
)
shift
goto :parse_args
:end_parse

echo [INFO] Build Type: %BUILD_TYPE%
echo [INFO] Generator: %GENERATOR%
echo.

REM Clean build if requested
if %CLEAN_BUILD%==1 (
    echo [INFO] Cleaning previous build...
    if exist build (
        rmdir /s /q build
        echo [INFO] Build directory removed
    )
    echo.
)

REM Create build directory
if not exist build (
    echo [INFO] Creating build directory...
    mkdir build
)

REM Configure CMake
echo [INFO] Configuring CMake...
echo.

cmake -B build -G %GENERATOR% -A %PLATFORM% ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    echo.
    echo Possible solutions:
    echo   1. Ensure Vulkan SDK is installed and VULKAN_SDK environment variable is set
    echo   2. Install GLFW, GLM, stb, and tinyobjloader
    echo   3. Check BUILD.md for detailed instructions
    echo.
    exit /b 1
)

echo.
echo [INFO] Configuration successful!
echo.

REM Build
echo [INFO] Building project...
echo.

cmake --build build --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed!
    echo Check the error messages above for details.
    echo.
    exit /b 1
)

echo.
echo ============================================================================
echo Build Successful!
echo ============================================================================
echo.
echo Executable location: build\bin\%BUILD_TYPE%\Vulkan101.exe
echo.
echo To run the application:
echo   cd build\bin\%BUILD_TYPE%
echo   Vulkan101.exe
echo.
echo Or use the run script:
echo   run.bat
echo.

goto :eof

:show_help
echo Usage: build.bat [options]
echo.
echo Options:
echo   --debug      Build in Debug mode (default: Release)
echo   --release    Build in Release mode
echo   --clean      Clean build directory before building
echo   --vs2019     Use Visual Studio 2019 generator
echo   --vs2022     Use Visual Studio 2022 generator (default)
echo   --help       Show this help message
echo.
echo Examples:
echo   build.bat                    # Build in Release mode with VS2022
echo   build.bat --debug            # Build in Debug mode
echo   build.bat --clean --release  # Clean and build in Release mode
echo.
exit /b 0
