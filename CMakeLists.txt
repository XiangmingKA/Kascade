cmake_minimum_required(VERSION 3.20)
project(Vulkan101 VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# Project Settings
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================================
# Dependencies
# ============================================================================

# Vulkan SDK
find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found! Please install the Vulkan SDK from https://vulkan.lunarg.com/")
endif()

message(STATUS "Vulkan found: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan library: ${Vulkan_LIBRARIES}")

# GLFW
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    # Try to find GLFW manually
    find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h
        PATHS
            "$ENV{GLFW_DIR}/include"
            "$ENV{USERPROFILE}/Documents/glfw-3.4.bin.WIN64/include"
            "C:/Program Files/GLFW/include"
        DOC "GLFW include directory"
    )

    find_library(GLFW_LIBRARY
        NAMES glfw3 glfw
        PATHS
            "$ENV{GLFW_DIR}/lib"
            "$ENV{USERPROFILE}/Documents/glfw-3.4.bin.WIN64/lib-vc2022"
            "C:/Program Files/GLFW/lib"
        DOC "GLFW library"
    )

    if(GLFW_INCLUDE_DIR AND GLFW_LIBRARY)
        set(GLFW_FOUND TRUE)
        message(STATUS "GLFW found: ${GLFW_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "GLFW not found! Please install GLFW from https://www.glfw.org/")
    endif()
endif()

# GLM
find_package(glm QUIET)
if(NOT glm_FOUND)
    find_path(GLM_INCLUDE_DIR glm/glm.hpp
        PATHS
            "$ENV{GLM_DIR}/include"
            "$ENV{USERPROFILE}/Documents/glm-1.0.1"
            "C:/Program Files/glm/include"
        DOC "GLM include directory"
    )

    if(GLM_INCLUDE_DIR)
        set(GLM_FOUND TRUE)
        message(STATUS "GLM found: ${GLM_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "GLM not found! Please install GLM from https://github.com/g-truc/glm")
    endif()
endif()

# stb (header-only)
find_path(STB_INCLUDE_DIR stb_image.h
    PATHS
        "$ENV{STB_DIR}"
        "$ENV{USERPROFILE}/Documents/stb-master"
        "${CMAKE_SOURCE_DIR}/external/stb"
    DOC "stb include directory"
)

if(NOT STB_INCLUDE_DIR)
    message(FATAL_ERROR "stb not found! Please clone https://github.com/nothings/stb")
endif()
message(STATUS "stb found: ${STB_INCLUDE_DIR}")

# tinyobjloader (header-only)
find_path(TINYOBJLOADER_INCLUDE_DIR tiny_obj_loader.h
    PATHS
        "$ENV{TINYOBJLOADER_DIR}"
        "$ENV{USERPROFILE}/Documents/tinyobjloader-release"
        "${CMAKE_SOURCE_DIR}/external/tinyobjloader"
    DOC "tinyobjloader include directory"
)

if(NOT TINYOBJLOADER_INCLUDE_DIR)
    message(FATAL_ERROR "tinyobjloader not found! Please download from https://github.com/tinyobjloader/tinyobjloader")
endif()
message(STATUS "tinyobjloader found: ${TINYOBJLOADER_INCLUDE_DIR}")

# tinygltf (header-only)
find_path(TINYGLTF_INCLUDE_DIR tiny_gltf.h
    PATHS
        "$ENV{TINYGLTF_DIR}"
        "$ENV{USERPROFILE}/Documents/tinygltf-release"
        "${CMAKE_SOURCE_DIR}/external/tinygltf"
    DOC "tinygltf include directory"
)

if(NOT TINYGLTF_INCLUDE_DIR)
    message(FATAL_ERROR "tinygltf not found! Please download from https://github.com/syoyo/tinygltf")
endif()
message(STATUS "tinygltf found: ${TINYGLTF_INCLUDE_DIR}")

# ============================================================================
# Source Files
# ============================================================================

set(SOURCES
    source/main.cpp
    source/Application.cpp
    source/Instance.cpp
    source/Surface.cpp
    source/PhysicalDevice.cpp
    source/Device.cpp
    source/Swapchain.cpp
    source/RenderPass.cpp
    source/Pipeline.cpp
    source/Framebuffer.cpp
    source/CommandBuffer.cpp
    source/SyncObjects.cpp
    source/Vertex.cpp
    source/Buffer.cpp
    source/VertexIndexBuffers.cpp
    source/UniformBuffers.cpp
    source/Texture.cpp
    source/Cubemap.cpp
    source/Descriptor.cpp
    source/Utilities.cpp
    source/GltfVertex.cpp
    source/GltfMaterial.cpp
    source/GltfNode.cpp
    source/GltfPrimitive.cpp
    source/GltfMesh.cpp
    source/GltfModel.cpp
    source/GltfDescriptors.cpp
)

set(HEADERS
    source/Application.h
    source/Instance.h
    source/Surface.h
    source/PhysicalDevice.h
    source/Device.h
    source/Swapchain.h
    source/RenderPass.h
    source/Pipeline.h
    source/Framebuffer.h
    source/CommandBuffer.h
    source/SyncObjects.h
    source/Vertex.h
    source/Buffer.h
    source/VertexIndexBuffers.h
    source/UniformBuffers.h
    source/Texture.h
    source/Cubemap.h
    source/Descriptor.h
    source/Utilities.h
    source/GltfVertex.h
    source/GltfMaterial.h
    source/GltfNode.h
    source/GltfPrimitive.h
    source/GltfMesh.h
    source/GltfModel.h
    source/GltfDescriptors.h
)

# ============================================================================
# Executable
# ============================================================================

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${Vulkan_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${STB_INCLUDE_DIR}
    ${TINYOBJLOADER_INCLUDE_DIR}
    ${TINYGLTF_INCLUDE_DIR}
)

# Link libraries
if(glfw3_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Vulkan::Vulkan
        glfw
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Vulkan::Vulkan
        ${GLFW_LIBRARY}
    )
endif()

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        VK_USE_PLATFORM_WIN32_KHR
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )

    # Add Windows libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE
        user32
        gdi32
    )
elseif(UNIX AND NOT APPLE)
    # Linux-specific settings
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        VK_USE_PLATFORM_XLIB_KHR
    )

    # Add Linux libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE
        dl
        pthread
        X11
        Xxf86vm
        Xrandr
        Xi
    )
elseif(APPLE)
    # macOS-specific settings
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        VK_USE_PLATFORM_MACOS_MVK
    )
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ============================================================================
# Shader Compilation
# ============================================================================

# Find glslc compiler (part of Vulkan SDK)
find_program(GLSLC glslc HINTS
    ${Vulkan_GLSLC_EXECUTABLE}
    "$ENV{VULKAN_SDK}/Bin"
    "C:/Program Files/VulkanSDK/Bin"
)

if(NOT GLSLC)
    message(WARNING "glslc not found! Shaders will not be compiled automatically. Please compile manually.")
else()
    message(STATUS "glslc found: ${GLSLC}")

    # Shader files
    set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
    set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/shaders)

    file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

    # Compile vertex shader
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_DIR}/vert.spv
        COMMAND ${GLSLC} ${SHADER_DIR}/shader.vert -o ${SHADER_OUTPUT_DIR}/vert.spv
        DEPENDS ${SHADER_DIR}/shader.vert
        COMMENT "Compiling vertex shader"
        VERBATIM
    )

    # Compile fragment shader
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_DIR}/frag.spv
        COMMAND ${GLSLC} ${SHADER_DIR}/shader.frag -o ${SHADER_OUTPUT_DIR}/frag.spv
        DEPENDS ${SHADER_DIR}/shader.frag
        COMMENT "Compiling fragment shader"
        VERBATIM
    )

    # Add custom target for shaders
    add_custom_target(shaders ALL
        DEPENDS
            ${SHADER_OUTPUT_DIR}/vert.spv
            ${SHADER_OUTPUT_DIR}/frag.spv
    )

    add_dependencies(${PROJECT_NAME} shaders)
endif()

# ============================================================================
# Copy Assets to Build Directory
# ============================================================================

# Copy models
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/models
        ${CMAKE_BINARY_DIR}/bin/models
    COMMENT "Copying models to build directory"
)

# Copy textures
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/textures
        ${CMAKE_BINARY_DIR}/bin/textures
    COMMENT "Copying textures to build directory"
)

# Copy compiled shaders (if they exist in source)
if(EXISTS ${CMAKE_SOURCE_DIR}/shaders/vert.spv)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            ${CMAKE_BINARY_DIR}/bin/shaders
        COMMENT "Copying pre-compiled shaders to build directory"
    )
endif()

# ============================================================================
# Installation (Optional)
# ============================================================================

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY models DESTINATION bin)
install(DIRECTORY textures DESTINATION bin)
install(DIRECTORY ${CMAKE_BINARY_DIR}/bin/shaders DESTINATION bin)

# ============================================================================
# Status Messages
# ============================================================================

message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project Name:       ${PROJECT_NAME}")
message(STATUS "  C++ Standard:       C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:         ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output Directory:   ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  Vulkan SDK:         ${Vulkan_INCLUDE_DIRS}")
message(STATUS "  GLFW:               ${GLFW_INCLUDE_DIR}")
message(STATUS "  GLM:                ${GLM_INCLUDE_DIR}")
message(STATUS "  stb:                ${STB_INCLUDE_DIR}")
message(STATUS "  tinyobjloader:      ${TINYOBJLOADER_INCLUDE_DIR}")
message(STATUS "  tinygltf:           ${TINYGLTF_INCLUDE_DIR}")
message(STATUS "")
