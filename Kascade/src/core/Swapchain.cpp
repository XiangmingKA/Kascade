#include "Swapchain.h"
#include <stdexcept>
#include <iostream>

Swapchain::Swapchain(VulkanInstance& vulkanInstance) : vulkanInstance(vulkanInstance), swapchain(VK_NULL_HANDLE) {}

Swapchain::~Swapchain() {
    Cleanup();
}

void Swapchain::Init() {
    CreateSwapchain();
}

void Swapchain::Cleanup() {
    if (swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(vulkanInstance.GetLogicalDevice(), swapchain, nullptr);
    }
}

void Swapchain::CreateSwapchain() {
    PickSwapchainSurfaceFormat();
    PickSwapchainExtent();

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = nullptr;  // 这里需要你传入窗口的 Vulkan surface
    createInfo.minImageCount = 2;
    createInfo.imageFormat = swapchainImageFormat;
    createInfo.imageExtent = swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (vkCreateSwapchainKHR(vulkanInstance.GetLogicalDevice(), &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain!");
    }

    // 获取交换链图像
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(vulkanInstance.GetLogicalDevice(), swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vulkanInstance.GetLogicalDevice(), swapchain, &imageCount, swapchainImages.data());
}

void Swapchain::PickSwapchainSurfaceFormat() {
    swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;  // 默认选择 BGRA 格式
}

void Swapchain::PickSwapchainExtent() {
    swapchainExtent.width = 800;  // 默认窗口大小
    swapchainExtent.height = 600;
}