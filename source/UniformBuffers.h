#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include "Buffer.h"

class Device;

class UniformBufferSet {
public:
    void create(const Device& device,
        size_t framesInFlight,
        VkDeviceSize uboSize);
    void destroy(const Device& device);
    void updateFrame(size_t frameIndex, const void* data, VkDeviceSize bytes);
    VkBuffer buffer(size_t i) const { return m_buffers[i].get(); }
    VkDeviceSize size(size_t) const { return m_uboSize; }
    size_t count() const { return m_buffers.size(); }
private:
    std::vector<Buffer> m_buffers;
    VkDeviceSize m_uboSize = 0;
};
