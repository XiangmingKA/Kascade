#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "Device.h"

// ============================================================================
// glTF Descriptor Management
// Manages multiple descriptor set layouts and pools for glTF rendering
// ============================================================================

// Set 0: Per-frame data (camera, lights)
// Set 1: Per-model data (transform and material buffers)
// Set 2: Textures (array of samplers)
// Set 3: Environment cubemap

class GltfDescriptorSetLayouts {
public:
    GltfDescriptorSetLayouts() = default;
    ~GltfDescriptorSetLayouts() = default;

    // Create all four descriptor set layouts
    void create(const Device& device);
    void destroy(const Device& device);

    // Accessors for individual layouts
    VkDescriptorSetLayout getPerFrameLayout() const { return m_perFrameLayout; }
    VkDescriptorSetLayout getPerModelLayout() const { return m_perModelLayout; }
    VkDescriptorSetLayout getTexturesLayout() const { return m_texturesLayout; }
    VkDescriptorSetLayout getEnvironmentLayout() const { return m_environmentLayout; }

    // Get all layouts as a vector (useful for pipeline creation)
    std::vector<VkDescriptorSetLayout> getAllLayouts() const {
        return { m_perFrameLayout, m_perModelLayout, m_texturesLayout, m_environmentLayout };
    }

private:
    VkDescriptorSetLayout m_perFrameLayout = VK_NULL_HANDLE;       // Set 0
    VkDescriptorSetLayout m_perModelLayout = VK_NULL_HANDLE;       // Set 1
    VkDescriptorSetLayout m_texturesLayout = VK_NULL_HANDLE;       // Set 2
    VkDescriptorSetLayout m_environmentLayout = VK_NULL_HANDLE;    // Set 3

    void createPerFrameLayout(const Device& device);
    void createPerModelLayout(const Device& device);
    void createTexturesLayout(const Device& device);
    void createEnvironmentLayout(const Device& device);
};

class GltfDescriptorPool {
public:
    GltfDescriptorPool() = default;
    ~GltfDescriptorPool() = default;

    // Create descriptor pool sized for glTF rendering
    // maxTextures: maximum number of textures to support (default 32)
    void create(const Device& device, uint32_t maxTextures = 32);
    void destroy(const Device& device);

    VkDescriptorPool get() const { return m_pool; }

private:
    VkDescriptorPool m_pool = VK_NULL_HANDLE;
};

class GltfDescriptorSets {
public:
    GltfDescriptorSets() = default;
    ~GltfDescriptorSets() = default;

    // Allocate all descriptor sets from the pool
    void allocate(const Device& device,
                  VkDescriptorPool pool,
                  const GltfDescriptorSetLayouts& layouts,
                  uint32_t framesInFlight);

    void destroy();

    // Get descriptor sets for a specific frame
    VkDescriptorSet getPerFrameSet(uint32_t frameIndex) const { return m_perFrameSets[frameIndex]; }
    VkDescriptorSet getPerModelSet(uint32_t frameIndex) const { return m_perModelSets[frameIndex]; }
    VkDescriptorSet getTexturesSet(uint32_t frameIndex) const { return m_texturesSets[frameIndex]; }
    VkDescriptorSet getEnvironmentSet(uint32_t frameIndex) const { return m_environmentSets[frameIndex]; }

    // Get all sets for a frame as array (for vkCmdBindDescriptorSets)
    std::vector<VkDescriptorSet> getAllSets(uint32_t frameIndex) const {
        return {
            m_perFrameSets[frameIndex],
            m_perModelSets[frameIndex],
            m_texturesSets[frameIndex],
            m_environmentSets[frameIndex]
        };
    }

private:
    std::vector<VkDescriptorSet> m_perFrameSets;       // One per frame in flight
    std::vector<VkDescriptorSet> m_perModelSets;       // One per frame in flight
    std::vector<VkDescriptorSet> m_texturesSets;       // One per frame in flight
    std::vector<VkDescriptorSet> m_environmentSets;    // One per frame in flight
};
