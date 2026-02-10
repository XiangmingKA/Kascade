#include "Buffer.h"
#include "Device.h"
#include "Utilities.h"
#include <stdexcept>
#include <cstring>

static uint32_t findMemoryTypeInternal(VkPhysicalDevice physical,
    uint32_t typeBits,
    VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(physical, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeBits & (1u << i)) &&
            (memProps.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    throw std::runtime_error("Buffer::findMemoryTypeInternal: no suitable memory type");
}

void Buffer::create(const Device& device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
    m_bufferSize = size;

    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device.get(), &info, nullptr, &m_buffer) != VK_SUCCESS) {
        throw std::runtime_error("Buffer::create: vkCreateBuffer failed");
    }

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(device.get(), m_buffer, &req);

    VkMemoryAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = findMemoryTypeInternal(device.physical(), req.memoryTypeBits, properties);

    if (vkAllocateMemory(device.get(), &alloc, nullptr, &m_memory) != VK_SUCCESS) {
        throw std::runtime_error("Buffer::create: vkAllocateMemory failed");
    }
    vkBindBufferMemory(device.get(), m_buffer, m_memory, 0);
}

void Buffer::createAndMap(const Device& device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
    create(device, size, usage, properties);
    map(device);
}

void Buffer::destroy(const Device& device) {
    if (m_mappedPtr) {
        vkUnmapMemory(device.get(), m_memory);
        m_mappedPtr = nullptr;
    }
    if (m_buffer) {
        vkDestroyBuffer(device.get(), m_buffer, nullptr);
        m_buffer = VK_NULL_HANDLE;
    }
    if (m_memory) {
        vkFreeMemory(device.get(), m_memory, nullptr);
        m_memory = VK_NULL_HANDLE;
    }
    m_bufferSize = 0;
}

void* Buffer::map(const Device& device, VkDeviceSize offset, VkDeviceSize size) {
    if (!m_mappedPtr) {
        if (vkMapMemory(device.get(), m_memory, offset, size, 0, &m_mappedPtr) != VK_SUCCESS) {
            throw std::runtime_error("Buffer::map failed");
        }
    }
    return m_mappedPtr;
}

void Buffer::unmap(const Device& device) {
    if (m_mappedPtr) {
        vkUnmapMemory(device.get(), m_memory);
        m_mappedPtr = nullptr;
    }
}

void Buffer::write(const void* src, VkDeviceSize size, VkDeviceSize dstOffset) {
    if (m_mappedPtr == nullptr)
    {
        throw std::runtime_error("Mapped memory is not available!");
    }
    if (size > m_bufferSize)
    {
        throw std::runtime_error("Write bytes exceeded allocated memory size!");
    }
    std::memcpy(m_mappedPtr, src, (size_t)size);
}

void Buffer::copyBuffer(const Device& device,
    VkCommandPool commandPool,
    VkQueue queue,
    VkBuffer src,
    VkBuffer dst,
    VkDeviceSize size)
{
    VkCommandBuffer cmd = beginSingleTimeCommands(device, commandPool);

    VkBufferCopy region{};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = size;
    vkCmdCopyBuffer(cmd, src, dst, 1, &region);

    endSingleTimeCommands(device, cmd, commandPool, queue);
}
