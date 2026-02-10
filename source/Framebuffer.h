#pragma once
#include <vulkan/vulkan.h>
#include "Device.h"
#include "Swapchain.h"
#include "RenderPass.h"
#include <vector>

class Framebuffer {
public:
    void create(const Device& device, const Swapchain& swapchain, const RenderPass& renderPass);
    void destroy(const Device& device);
    std::vector<VkFramebuffer> get() const { return m_framebuffers; }
private:
    std::vector<VkFramebuffer> m_framebuffers;
    VkImage m_colorImage;
    VkDeviceMemory m_colorImageMemory;
    VkImageView m_colorImageView;
    VkImage m_depthImage;
    VkDeviceMemory m_depthImageMemory;
    VkImageView m_depthImageView;
    void createColorResources(const Device& device, VkFormat colorFormat, VkSampleCountFlagBits msaa, VkExtent2D extent);
    void createDepthResources(const Device& device, VkFormat colorFormat, VkSampleCountFlagBits msaa, VkExtent2D extent);
    void destroyColorResources(const Device& device);
    void destroyDepthResources(const Device& device);
};

