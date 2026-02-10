#include "VertexIndexBuffers.h"
#include "Device.h"

void VertexBuffer::createFrom(const Device& device,
    VkCommandPool commandPool,
    VkQueue transferQueue,
    const void* vertexData,
    VkDeviceSize bytes)
{
    Buffer staging;
    staging.createAndMap(device, bytes,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    staging.write(vertexData, bytes);
    staging.unmap(device);

    m_gpu.create(device, bytes,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer::copyBuffer(device, commandPool, transferQueue, staging.get(), m_gpu.get(), bytes);
    staging.destroy(device);
}

void VertexBuffer::createFromVector(const Device& device,
    VkCommandPool commandPool,
    VkQueue transferQueue,
    const std::vector<uint8_t>& rawBytes)
{
    createFrom(device, commandPool, transferQueue, rawBytes.data(), static_cast<VkDeviceSize>(rawBytes.size()));
}

void VertexBuffer::destroy(const Device& device) {
    m_gpu.destroy(device);
}

void IndexBuffer::createFrom(const Device& device,
    VkCommandPool commandPool,
    VkQueue transferQueue,
    const void* indexData,
    VkDeviceSize bytes)
{
    Buffer staging;
    staging.createAndMap(device, bytes,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    staging.write(indexData, bytes);
    staging.unmap(device);

    m_gpu.create(device, bytes,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer::copyBuffer(device, commandPool, transferQueue, staging.get(), m_gpu.get(), bytes);
    staging.destroy(device);
}

void IndexBuffer::destroy(const Device& device) {
    m_gpu.destroy(device);
}
