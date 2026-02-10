#pragma once
#include <vulkan/vulkan.h>
#include "Device.h"

class RenderPass {
public:
    void create(const Device& device, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits msaa);
    void destroy(const Device& device);
    VkRenderPass get() const { return m_renderPass; }
private:
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
};