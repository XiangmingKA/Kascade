#include "Surface.h"
#include <stdexcept>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

void Surface::create(const VkInstance instance, GLFWwindow* window)
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &m_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void Surface::destroy(const VkInstance instance)
{
    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
}
