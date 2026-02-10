#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "Device.h"
#include <GLFW/glfw3.h>

class Surface {
public:
    void create(VkInstance instance, GLFWwindow* window);
    void destroy(VkInstance instance);
    VkSurfaceKHR get() const { return m_surface; }
private:
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};
