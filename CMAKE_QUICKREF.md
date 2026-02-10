# CMake Quick Reference for Vulkan101

## Basic Commands

### Configure
```bash
# Windows (Visual Studio)
cmake -B build -G "Visual Studio 17 2022" -A x64

# Linux/macOS
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

### Build
```bash
# Windows
cmake --build build --config Release

# Linux/macOS
cmake --build build
```

### Clean
```bash
# Remove build directory
rm -rf build

# Or on Windows
rmdir /s /q build
```

## Build Configurations

### Debug Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Release Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Release with Debug Info
```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

## Specify Library Paths

If CMake cannot find dependencies automatically:

```bash
cmake -B build \
    -DGLFW_INCLUDE_DIR=/path/to/glfw/include \
    -DGLFW_LIBRARY=/path/to/glfw/lib/glfw3.lib \
    -DGLM_INCLUDE_DIR=/path/to/glm \
    -DSTB_INCLUDE_DIR=/path/to/stb \
    -DTINYOBJLOADER_INCLUDE_DIR=/path/to/tinyobjloader
```

## Platform-Specific

### Windows

```cmd
# Visual Studio 2022
cmake -B build -G "Visual Studio 17 2022" -A x64

# Visual Studio 2019
cmake -B build -G "Visual Studio 16 2019" -A x64

# Ninja (faster builds)
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Linux

```bash
# Default (Unix Makefiles)
cmake -B build -DCMAKE_BUILD_TYPE=Release
make -C build -j$(nproc)

# Ninja
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

### macOS

```bash
# Xcode
cmake -B build -G "Xcode"
cmake --build build --config Release

# Unix Makefiles
cmake -B build -DCMAKE_BUILD_TYPE=Release
make -C build -j$(sysctl -n hw.ncpu)
```

## Advanced Options

### Verbose Build
```bash
cmake --build build --verbose
```

### Parallel Build
```bash
# Windows
cmake --build build -j 8

# Linux
cmake --build build -j$(nproc)

# macOS
cmake --build build -j$(sysctl -n hw.ncpu)
```

### Install
```bash
cmake --install build --prefix /path/to/install
```

## Useful CMake Variables

```bash
# Set C++ compiler
cmake -B build -DCMAKE_CXX_COMPILER=g++

# Set C compiler
cmake -B build -DCMAKE_C_COMPILER=gcc

# Set install prefix
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local

# Enable verbose makefiles
cmake -B build -DCMAKE_VERBOSE_MAKEFILE=ON
```

## Environment Variables

### Vulkan SDK
```bash
# Windows
set VULKAN_SDK=C:\VulkanSDK\1.3.xxx.x

# Linux/macOS
export VULKAN_SDK=/path/to/vulkan-sdk
source $VULKAN_SDK/setup-env.sh
```

### Custom Paths
```bash
# Windows
set GLFW_DIR=C:\path\to\glfw
set GLM_DIR=C:\path\to\glm

# Linux/macOS
export GLFW_DIR=/path/to/glfw
export GLM_DIR=/path/to/glm
```

## Troubleshooting

### View CMake Configuration
```bash
cmake -B build -LA
```

### View Detailed Configuration
```bash
cmake -B build -LAH
```

### Check CMake Version
```bash
cmake --version
```

### Clear CMake Cache
```bash
rm build/CMakeCache.txt
cmake -B build
```

## Build Scripts

The project includes convenient build scripts:

### Windows
```cmd
build.bat              # Build Release
build.bat --debug      # Build Debug
build.bat --clean      # Clean and build
run.bat                # Run Release build
```

### Linux/macOS
```bash
./build.sh             # Build Release
./build.sh --debug     # Build Debug
./build.sh --clean     # Clean and build
./run.sh               # Run Release build
```

## Project-Specific Targets

### Build Shaders Only
```bash
cmake --build build --target shaders
```

### All Targets
```bash
cmake --build build --target all
```

## Output Locations

After building, files are organized as:

```
build/
├── bin/                    # Executables
│   ├── Vulkan101[.exe]    # Main executable
│   ├── shaders/           # Compiled SPIR-V shaders
│   ├── models/            # Copied model files
│   └── textures/          # Copied texture files
└── lib/                   # Libraries (if any)
```

## Common Workflows

### First Build
```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run
cd build/bin
./Vulkan101
```

### Rebuild After Code Changes
```bash
# Just build (CMake detects changes)
cmake --build build
```

### Complete Clean Rebuild
```bash
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Update Dependencies
```bash
# Reconfigure with new paths
cmake -B build \
    -DGLFW_INCLUDE_DIR=/new/path/to/glfw/include \
    -DGLFW_LIBRARY=/new/path/to/glfw/lib/glfw3.lib

# Rebuild
cmake --build build
```

## Performance Tips

1. **Use Ninja** for faster builds:
   ```bash
   cmake -B build -G "Ninja"
   ```

2. **Enable parallel builds**:
   ```bash
   cmake --build build -j8
   ```

3. **Use Release builds** for testing performance:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```

4. **Precompiled headers** (enabled in CMakeLists.txt)

## IDE Integration

### Visual Studio Code
1. Install CMake Tools extension
2. Press `Ctrl+Shift+P` → "CMake: Configure"
3. Press `F7` to build

### CLion
1. Open project folder
2. CLion automatically detects CMakeLists.txt
3. Build configuration in bottom toolbar

### Visual Studio
1. Open as CMake project (File → Open → CMake)
2. Build from Build menu

## Help & Documentation

```bash
# CMake help
cmake --help

# Generator help
cmake --help-generator-list

# Variable help
cmake --help-variable-list

# Module help
cmake --help-module FindVulkan
```
