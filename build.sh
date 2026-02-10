#!/bin/bash
# ============================================================================
# Vulkan101 Build Script for Linux/macOS
# ============================================================================

set -e  # Exit on error

echo ""
echo "============================================================================"
echo "Vulkan101 Build Script"
echo "============================================================================"
echo ""

# Default values
BUILD_TYPE="Release"
CLEAN_BUILD=0
VERBOSE=0
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Parse command line arguments
show_help() {
    cat << EOF
Usage: ./build.sh [options]

Options:
    --debug         Build in Debug mode (default: Release)
    --release       Build in Release mode
    --clean         Clean build directory before building
    --verbose       Show verbose build output
    -j <N>          Use N parallel jobs (default: auto-detect)
    --help          Show this help message

Examples:
    ./build.sh                      # Build in Release mode
    ./build.sh --debug              # Build in Debug mode
    ./build.sh --clean --release    # Clean and build in Release mode
    ./build.sh -j 8                 # Build with 8 parallel jobs

EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        --verbose)
            VERBOSE=1
            shift
            ;;
        -j)
            JOBS="$2"
            shift 2
            ;;
        --help)
            show_help
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo "[INFO] Build Type: $BUILD_TYPE"
echo "[INFO] Parallel Jobs: $JOBS"
echo ""

# Check if CMake is available
if ! command -v cmake &> /dev/null; then
    echo "[ERROR] CMake not found! Please install CMake."
    echo ""
    echo "Ubuntu/Debian: sudo apt install cmake"
    echo "macOS:         brew install cmake"
    echo ""
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1)
echo "[INFO] $CMAKE_VERSION"
echo ""

# Clean build if requested
if [ $CLEAN_BUILD -eq 1 ]; then
    echo "[INFO] Cleaning previous build..."
    rm -rf build
    echo "[INFO] Build directory removed"
    echo ""
fi

# Create build directory
if [ ! -d "build" ]; then
    echo "[INFO] Creating build directory..."
    mkdir build
fi

# Configure CMake
echo "[INFO] Configuring CMake..."
echo ""

CMAKE_ARGS=(
    -B build
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
)

# Add verbose flag if requested
if [ $VERBOSE -eq 1 ]; then
    CMAKE_ARGS+=(-DCMAKE_VERBOSE_MAKEFILE=ON)
fi

cmake "${CMAKE_ARGS[@]}"

if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] CMake configuration failed!"
    echo ""
    echo "Possible solutions:"
    echo "  1. Ensure Vulkan SDK is installed and in PATH"
    echo "  2. Install GLFW, GLM, stb, and tinyobjloader"
    echo "  3. Check BUILD.md for detailed instructions"
    echo ""
    exit 1
fi

echo ""
echo "[INFO] Configuration successful!"
echo ""

# Build
echo "[INFO] Building project..."
echo ""

BUILD_ARGS=(--build build --config "$BUILD_TYPE" -j "$JOBS")

if [ $VERBOSE -eq 1 ]; then
    BUILD_ARGS+=(--verbose)
fi

cmake "${BUILD_ARGS[@]}"

if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] Build failed!"
    echo "Check the error messages above for details."
    echo ""
    exit 1
fi

echo ""
echo "============================================================================"
echo "Build Successful!"
echo "============================================================================"
echo ""
echo "Executable location: build/bin/Vulkan101"
echo ""
echo "To run the application:"
echo "  cd build/bin"
echo "  ./Vulkan101"
echo ""
echo "Or use the run script:"
echo "  ./run.sh"
echo ""
