#pragma once
#include <vulkan/vulkan.h>
#include "PhysicalDevice.h"

class Device {
public:
    void create(const PhysicalDevice& physicalDevice, bool enableValidationLayers);
    void destroy();

    VkDevice get() const { return m_device; }
    VkPhysicalDevice physical() const { return m_physicalDevice; }
    VkQueue graphicsQ() const { return m_graphicsQueue; }
    VkQueue presentQ() const { return m_presentQueue; }
    QueueFamilyIndices queues() const { return m_queueIndices; }

private:
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    QueueFamilyIndices m_queueIndices;
};

