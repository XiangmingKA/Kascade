#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "Device.h"

class PipelineLayoutRAII {
public:
    void create(const Device& device, const std::vector<VkDescriptorSetLayout>& setLayouts, const std::vector<VkPushConstantRange>& pushConstants);
    void destroy(const Device& device);
    VkPipelineLayout get() const { return m_layout; }
private:
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
};

class GraphicsPipeline {
public:
    void create(const Device& device, VkRenderPass renderPass, VkPipelineLayout layout,
        VkExtent2D extent, VkSampleCountFlagBits msaa, VkDescriptorSetLayout descriptorSetLayout,
        VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void destroy(const Device& device);
    VkPipeline get() const { return m_pipeline; }
    VkPipelineLayout getLayout() const { return m_pipelineLayout; }
private:
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
};

