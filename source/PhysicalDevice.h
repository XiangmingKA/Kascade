#pragma once
#include <vulkan/vulkan.h>
#include "Instance.h"
#include <optional>
#include <vector>
#include <string>

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class PhysicalDevice {
public:
    void pick(VkInstance instance, VkSurfaceKHR surface);
    VkPhysicalDevice get() const { return m_physicalDevice; }
    QueueFamilyIndices queues() const { return m_queueIndices; }
    VkSampleCountFlagBits msaa() const { return m_msaaSamples; }
    VkFormat depthFormat() const { return m_depthFormat; }
    static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
private:
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices m_queueIndices;
    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};
