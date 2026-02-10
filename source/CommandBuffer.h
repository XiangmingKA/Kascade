#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "Device.h"
#include "PhysicalDevice.h"
#include "Swapchain.h"
#include "Framebuffer.h"
#include "Pipeline.h"
#include "VertexIndexBuffers.h"
#include "Descriptor.h"

class CommandPool {
public:
    void create(const Device& device, VkCommandPoolCreateFlags flags, VkSurfaceKHR surface);
    void destroy(const Device& device);
    VkCommandPool get() const { return m_pool; }
private:
    VkCommandPool m_pool = VK_NULL_HANDLE;
};

class CommandBuffer {
public:
    void create(const Device& device, VkCommandPool commandPool, uint32_t count);
    void destroy(const Device& device);
    void record(uint32_t imageIndex, uint32_t currentFrame, VkRenderPass renderPass, Swapchain swapchain,
        Framebuffer framebuffer, GraphicsPipeline graphicsPipeline, VertexBuffer vertexBuffer, IndexBuffer indexBuffer,
        DescriptorSet descriptorSets);
    void reset(uint32_t index);
    std::vector<VkCommandBuffer> get() const { return m_commandBuffers; }
    VkCommandBuffer& operator[](size_t index) { return m_commandBuffers[index]; }
private:
    std::vector<VkCommandBuffer> m_commandBuffers;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
};

