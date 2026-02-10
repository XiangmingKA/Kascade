#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include "Buffer.h"

class Device;

class VertexBuffer {
public:
    void createFrom(const Device& device,
        VkCommandPool commandPool,
        VkQueue transferQueue,
        const void* vertexData,
        VkDeviceSize bytes);
    void createFromVector(const Device& device,
        VkCommandPool commandPool,
        VkQueue transferQueue,
        const std::vector<uint8_t>& rawBytes);
    template<typename T>
    void createFromVector(const Device& device,
        VkCommandPool commandPool,
        VkQueue transferQueue,
        const std::vector<T>& vec) {
        createFrom(device, commandPool, transferQueue, vec.data(), sizeof(T) * vec.size());
    }
    void destroy(const Device& device);

    VkBuffer get() const { return m_gpu.get(); }
    VkDeviceSize size() const { return m_gpu.size(); }

private:
    Buffer m_gpu;
};

class IndexBuffer {
public:
    void createFrom(const Device& device,
        VkCommandPool commandPool,
        VkQueue transferQueue,
        const void* indexData,
        VkDeviceSize bytes);
    template<typename T>
    void createFromVector(const Device& device,
        VkCommandPool commandPool,
        VkQueue transferQueue,
        const std::vector<T>& vec) {
        createFrom(device, commandPool, transferQueue, vec.data(), sizeof(T) * vec.size());
    }
    void destroy(const Device& device);

    VkBuffer get() const { return m_gpu.get(); }
    VkDeviceSize size() const { return m_gpu.size(); }

private:
    Buffer m_gpu;
};
