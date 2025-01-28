#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class VulkanInstance {
public:
    VulkanInstance();
    ~VulkanInstance();

    void Init();
    void Cleanup();

    VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
    VkDevice GetLogicalDevice() const { return logicalDevice; }

private:
    VkInstance instance;
    VkDevice logicalDevice;
    VkPhysicalDevice physicalDevice;
    VkQueue graphicsQueue;

    void CreateInstance();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
};