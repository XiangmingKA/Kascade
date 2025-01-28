cmake_minimum_required(VERSION 3.20)
project(Kascade)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)

# 添加 Vulkan SDK
find_package(Vulkan REQUIRED)

# 添加第三方库路径
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Don't build GLFW docs")
add_subdirectory(third_party/glfw)

# 包含目录
include_directories(
    ${Vulkan_INCLUDE_DIRS}
    third_party/glfw/include
    src/
)

# 源代码文件
file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h)
file(GLOB_RECURSE ADDITIONAL_SRC_FILES src/core/Swapchain.cpp)

# 添加所有源文件
list(APPEND SRC_FILES ${ADDITIONAL_SRC_FILES})

# 可执行文件
add_executable(Kascade ${SRC_FILES})

# 链接库
target_link_libraries(Kascade Vulkan::Vulkan glfw)