#!/bin/bash
# ============================================================================
# Vulkan101 Run Script for Linux/macOS
# ============================================================================

set -e  # Exit on error

echo ""
echo "============================================================================"
echo "Vulkan101 Run Script"
echo "============================================================================"
echo ""

# Default values
BUILD_TYPE="Release"

# Parse command line arguments
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
        --help)
            echo "Usage: ./run.sh [options]"
            echo ""
            echo "Options:"
            echo "    --debug     Run Debug build"
            echo "    --release   Run Release build (default)"
            echo "    --help      Show this help message"
            echo ""
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

EXECUTABLE="build/bin/Vulkan101"

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "[ERROR] Executable not found: $EXECUTABLE"
    echo ""
    echo "Please build the project first:"
    echo "  ./build.sh"
    echo ""
    echo "Or build in the correct configuration:"
    echo "  ./build.sh --$BUILD_TYPE"
    echo ""
    exit 1
fi

# Check if assets are in place
if [ ! -d "build/bin/models" ]; then
    echo "[WARNING] Models directory not found in build output"
    echo "Assets may not have been copied correctly"
fi

if [ ! -d "build/bin/textures" ]; then
    echo "[WARNING] Textures directory not found in build output"
    echo "Assets may not have been copied correctly"
fi

if [ ! -d "build/bin/shaders" ]; then
    echo "[WARNING] Shaders directory not found in build output"
    echo "Shaders may not have been compiled or copied"
fi

echo "[INFO] Running Vulkan101 ($BUILD_TYPE build)..."
echo ""

# Change to the executable directory so it can find assets
cd build/bin

# Run the application
./Vulkan101

# Capture exit code
EXIT_CODE=$?

# Return to original directory
cd ../..

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo "[INFO] Application exited successfully"
else
    echo "[ERROR] Application exited with code: $EXIT_CODE"
fi
echo ""

exit $EXIT_CODE
