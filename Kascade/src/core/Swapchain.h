#pragma once
#include <vulkan/vulkan.h>
#include "VulkanInstance.h"

class Swapchain {
public:
    Swapchain(VulkanInstance& vulkanInstance);
    ~Swapchain();

    void Init();
    void Cleanup();

    VkSwapchainKHR GetSwapchain() const { return swapchain; }

private:
    VulkanInstance& vulkanInstance;
    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImage> swapchainImages;

    void CreateSwapchain();
    void PickSwapchainSurfaceFormat();
    void PickSwapchainExtent();
};
