#pragma once
#include <vulkan/vulkan.h>
#include "Device.h"

void createImage(const Device& device, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSample,
    VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
    VkImage& image, VkDeviceMemory& imageMemory);

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

VkImageView createImageView(const Device& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
VkFormat findSupportedFormat(const Device& device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
VkFormat findDepthFormat(const Device& device);
VkCommandBuffer beginSingleTimeCommands(const Device& device, VkCommandPool commandPool);
void endSingleTimeCommands(const Device& device, VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue);