#include "Descriptor.h"
#include <stdexcept>
#include <array>

void DescriptorSetLayoutRAII::create(const Device& device)
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutBinding samplerLayoutBinding1{};
    samplerLayoutBinding1.binding = 1;
    samplerLayoutBinding1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding1.descriptorCount = 1;
    samplerLayoutBinding1.pImmutableSamplers = nullptr;
    samplerLayoutBinding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding2{};
    samplerLayoutBinding2.binding = 2;
    samplerLayoutBinding2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding2.descriptorCount = 1;
    samplerLayoutBinding2.pImmutableSamplers = nullptr;
    samplerLayoutBinding2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding3{};
    samplerLayoutBinding3.binding = 3;
    samplerLayoutBinding3.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding3.descriptorCount = 1;
    samplerLayoutBinding3.pImmutableSamplers = nullptr;
    samplerLayoutBinding3.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding4{};
    samplerLayoutBinding4.binding = 4;
    samplerLayoutBinding4.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding4.descriptorCount = 1;
    samplerLayoutBinding4.pImmutableSamplers = nullptr;
    samplerLayoutBinding4.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding5{};
    samplerLayoutBinding5.binding = 5;
    samplerLayoutBinding5.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding5.descriptorCount = 1;
    samplerLayoutBinding5.pImmutableSamplers = nullptr;
    samplerLayoutBinding5.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding6{};
    samplerLayoutBinding6.binding = 6;
    samplerLayoutBinding6.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding6.descriptorCount = 1;
    samplerLayoutBinding6.pImmutableSamplers = nullptr;
    samplerLayoutBinding6.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 7> bindings = { 
        uboLayoutBinding, 
        samplerLayoutBinding1,
        samplerLayoutBinding2,
        samplerLayoutBinding3, 
        samplerLayoutBinding4,
        samplerLayoutBinding5,
        samplerLayoutBinding6 };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device.get(), &layoutInfo, nullptr, &m_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void DescriptorSetLayoutRAII::destroy(const Device& device)
{
    if (m_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device.get(), m_layout, nullptr);
        m_layout = VK_NULL_HANDLE;
    }
}

const int kMaxFramesInFlight = 2;

void DescriptorPoolRAII::create(const Device& device)
{
    std::array<VkDescriptorPoolSize, 7> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight);
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight);
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[3].descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight);
    poolSizes[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[4].descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight);
    poolSizes[5].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[5].descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight);
    poolSizes[6].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[6].descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(kMaxFramesInFlight);
    if (vkCreateDescriptorPool(device.get(), &poolInfo, nullptr, &m_pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void DescriptorPoolRAII::destroy(const Device& device)
{
    if (m_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device.get(), m_pool, nullptr);
        m_pool = VK_NULL_HANDLE;
    }
}

void DescriptorSet::create(const Device& device, VkDescriptorPool pool, VkDescriptorSetLayout layout)
{
    std::vector<VkDescriptorSetLayout> layouts(kMaxFramesInFlight, layout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(kMaxFramesInFlight);
    allocInfo.pSetLayouts = layouts.data();
    m_descriptorSets.resize(kMaxFramesInFlight);
    if (vkAllocateDescriptorSets(device.get(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
}

void DescriptorSet::destroy(const Device& device)
{
    m_descriptorSets.clear();
}