#include "GltfDescriptors.h"
#include <stdexcept>

// ============================================================================
// GltfDescriptorSetLayouts Implementation
// ============================================================================

void GltfDescriptorSetLayouts::create(const Device& device) {
    createPerFrameLayout(device);
    createPerModelLayout(device);
    createTexturesLayout(device);
    createEnvironmentLayout(device);
}

void GltfDescriptorSetLayouts::destroy(const Device& device) {
    if (m_perFrameLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device.get(), m_perFrameLayout, nullptr);
        m_perFrameLayout = VK_NULL_HANDLE;
    }
    if (m_perModelLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device.get(), m_perModelLayout, nullptr);
        m_perModelLayout = VK_NULL_HANDLE;
    }
    if (m_texturesLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device.get(), m_texturesLayout, nullptr);
        m_texturesLayout = VK_NULL_HANDLE;
    }
    if (m_environmentLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device.get(), m_environmentLayout, nullptr);
        m_environmentLayout = VK_NULL_HANDLE;
    }
}

void GltfDescriptorSetLayouts::createPerFrameLayout(const Device& device) {
    // Set 0, Binding 0: UBO with camera and light data
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboBinding;

    if (vkCreateDescriptorSetLayout(device.get(), &layoutInfo, nullptr, &m_perFrameLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create glTF per-frame descriptor set layout");
    }
}

void GltfDescriptorSetLayouts::createPerModelLayout(const Device& device) {
    // Set 1, Binding 0: Storage buffer for node transforms
    VkDescriptorSetLayoutBinding transformBinding{};
    transformBinding.binding = 0;
    transformBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    transformBinding.descriptorCount = 1;
    transformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    transformBinding.pImmutableSamplers = nullptr;

    // Set 1, Binding 1: Storage buffer for materials
    VkDescriptorSetLayoutBinding materialBinding{};
    materialBinding.binding = 1;
    materialBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    materialBinding.descriptorCount = 1;
    materialBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    materialBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding bindings[] = { transformBinding, materialBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device.get(), &layoutInfo, nullptr, &m_perModelLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create glTF per-model descriptor set layout");
    }
}

void GltfDescriptorSetLayouts::createTexturesLayout(const Device& device) {
    // Set 2, Binding 0: Array of texture samplers (up to 32)
    VkDescriptorSetLayoutBinding textureArrayBinding{};
    textureArrayBinding.binding = 0;
    textureArrayBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureArrayBinding.descriptorCount = 32;  // Support up to 32 textures
    textureArrayBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    textureArrayBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &textureArrayBinding;

    if (vkCreateDescriptorSetLayout(device.get(), &layoutInfo, nullptr, &m_texturesLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create glTF textures descriptor set layout");
    }
}

void GltfDescriptorSetLayouts::createEnvironmentLayout(const Device& device) {
    // Set 3, Binding 0: Environment cubemap for IBL
    VkDescriptorSetLayoutBinding cubemapBinding{};
    cubemapBinding.binding = 0;
    cubemapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    cubemapBinding.descriptorCount = 1;
    cubemapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    cubemapBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &cubemapBinding;

    if (vkCreateDescriptorSetLayout(device.get(), &layoutInfo, nullptr, &m_environmentLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create glTF environment descriptor set layout");
    }
}

// ============================================================================
// GltfDescriptorPool Implementation
// ============================================================================

void GltfDescriptorPool::create(const Device& device, uint32_t maxTextures) {
    constexpr uint32_t kMaxFramesInFlight = 2;

    // Pool sizes for all descriptor types we'll use
    VkDescriptorPoolSize poolSizes[4];

    // Uniform buffers (per-frame UBO)
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = kMaxFramesInFlight;

    // Storage buffers (transform + material buffers)
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = kMaxFramesInFlight * 2;  // 2 storage buffers per frame

    // Texture array (one array per frame)
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = kMaxFramesInFlight * maxTextures;

    // Environment cubemap
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[3].descriptorCount = kMaxFramesInFlight;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 4;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = kMaxFramesInFlight * 4;  // 4 descriptor sets per frame

    if (vkCreateDescriptorPool(device.get(), &poolInfo, nullptr, &m_pool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create glTF descriptor pool");
    }
}

void GltfDescriptorPool::destroy(const Device& device) {
    if (m_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device.get(), m_pool, nullptr);
        m_pool = VK_NULL_HANDLE;
    }
}

// ============================================================================
// GltfDescriptorSets Implementation
// ============================================================================

void GltfDescriptorSets::allocate(const Device& device,
                                   VkDescriptorPool pool,
                                   const GltfDescriptorSetLayouts& layouts,
                                   uint32_t framesInFlight) {
    // Resize vectors to hold one set per frame
    m_perFrameSets.resize(framesInFlight);
    m_perModelSets.resize(framesInFlight);
    m_texturesSets.resize(framesInFlight);
    m_environmentSets.resize(framesInFlight);

    // Allocate per-frame sets
    {
        std::vector<VkDescriptorSetLayout> setLayouts(framesInFlight, layouts.getPerFrameLayout());
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = framesInFlight;
        allocInfo.pSetLayouts = setLayouts.data();

        if (vkAllocateDescriptorSets(device.get(), &allocInfo, m_perFrameSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate glTF per-frame descriptor sets");
        }
    }

    // Allocate per-model sets
    {
        std::vector<VkDescriptorSetLayout> setLayouts(framesInFlight, layouts.getPerModelLayout());
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = framesInFlight;
        allocInfo.pSetLayouts = setLayouts.data();

        if (vkAllocateDescriptorSets(device.get(), &allocInfo, m_perModelSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate glTF per-model descriptor sets");
        }
    }

    // Allocate texture sets
    {
        std::vector<VkDescriptorSetLayout> setLayouts(framesInFlight, layouts.getTexturesLayout());
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = framesInFlight;
        allocInfo.pSetLayouts = setLayouts.data();

        if (vkAllocateDescriptorSets(device.get(), &allocInfo, m_texturesSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate glTF texture descriptor sets");
        }
    }

    // Allocate environment sets
    {
        std::vector<VkDescriptorSetLayout> setLayouts(framesInFlight, layouts.getEnvironmentLayout());
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = framesInFlight;
        allocInfo.pSetLayouts = setLayouts.data();

        if (vkAllocateDescriptorSets(device.get(), &allocInfo, m_environmentSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate glTF environment descriptor sets");
        }
    }
}

void GltfDescriptorSets::destroy() {
    // Descriptor sets are automatically freed when the pool is destroyed
    m_perFrameSets.clear();
    m_perModelSets.clear();
    m_texturesSets.clear();
    m_environmentSets.clear();
}
