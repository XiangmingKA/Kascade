#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "Device.h"

class DescriptorSetLayoutRAII {
public:
    void create(const Device& device);
    void destroy(const Device& device);
    VkDescriptorSetLayout get() const { return m_layout; }
private:
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
};

class DescriptorPoolRAII {
public:
    void create(const Device& device);
    void destroy(const Device& device);
    VkDescriptorPool get() const { return m_pool; }
private:
    VkDescriptorPool m_pool = VK_NULL_HANDLE;
};

class DescriptorSet {
public:
    void create(const Device& device, VkDescriptorPool pool, VkDescriptorSetLayout layout);
    void destroy(const Device& device);
    std::vector<VkDescriptorSet> get() const { return m_descriptorSets; }
    VkDescriptorSet& operator[](size_t index) { return m_descriptorSets[index]; }
private:
    std::vector<VkDescriptorSet> m_descriptorSets;
};
