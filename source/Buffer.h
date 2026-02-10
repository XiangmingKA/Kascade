#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>

class Device;

// RAII wrapper for VkBuffer and VkDeviceMemory
class Buffer {
public:
    Buffer() = default;
    ~Buffer() = default; // Call destroy() manually

    void create(const Device& device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);

    void createAndMap(const Device& device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);

    void destroy(const Device& device);

    void* map(const Device& device, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
    void  unmap(const Device& device);
    void  write(const void* src, VkDeviceSize bytes, VkDeviceSize dstOffset = 0);

    static void copyBuffer(const Device& device,
        VkCommandPool commandPool,
        VkQueue queue,
        VkBuffer src,
        VkBuffer dst,
        VkDeviceSize size);

    VkBuffer       get() const { return m_buffer; }
    VkDeviceMemory getMemory() const { return m_memory; }
    VkDeviceSize   size() const { return m_bufferSize; }
    bool           mapped() const { return m_mappedPtr != nullptr; }
    void* mappedPtrRaw() const { return m_mappedPtr; }

private:
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_bufferSize = 0;
    void* m_mappedPtr = nullptr;
};
