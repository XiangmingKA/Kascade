#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "Device.h"
#include <GLFW/glfw3.h>

class Swapchain {
public:
    void create(const Device& device, VkSurfaceKHR surface, GLFWwindow* window);
    void recreate(const Device& device, VkSurfaceKHR surface, GLFWwindow* window);
    void destroy(const Device& device);
    VkFormat imageFormat() const { return m_format; }
    VkExtent2D extent() const { return m_extent; }
    const std::vector<VkImageView>& imageViews() const { return m_views; }
    VkSwapchainKHR get() const { return m_swapchain; }
    VkFormat depthFormat();
    VkSampleCountFlagBits msaa() const { return m_msaaSamples; }
private:
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_views;
    VkFormat m_format{};
    VkExtent2D m_extent{};
    void createImageViews(const Device& device);
    Device m_device;
    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
};