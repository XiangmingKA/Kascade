#include "UniformBuffers.h"
#include "Device.h"

void UniformBufferSet::create(const Device& device,
    size_t framesInFlight,
    VkDeviceSize sizeBytes)
{
    m_uboSize = sizeBytes;
    m_buffers.resize(framesInFlight);
    for (auto& b : m_buffers) {
        b.createAndMap(device, m_uboSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
}

void UniformBufferSet::destroy(const Device& device) {
    for (auto& b : m_buffers) b.destroy(device);
    m_buffers.clear();
    m_uboSize = 0;
}

void UniformBufferSet::updateFrame(size_t frameIndex, const void* data, VkDeviceSize bytes) {
    m_buffers[frameIndex].write(data, bytes);
}
