#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>

class Device;

class Texture {
public:
    Texture() = default;
    virtual ~Texture() = default;

    // Create a 2D texture from pixel data (RGBA8 example), optionally generate mipmaps
    void createFromPixels(const Device& device,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        const void* pixels,
        uint32_t width,
        uint32_t height,
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
        bool generateMipmaps = true,
        VkFilter samplerFilter = VK_FILTER_LINEAR,
        VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

    virtual void destroy(const Device& device);

    uint32_t mipLevels() const { return m_mipLevels; }
    VkImage image() const { return m_image; }
    VkDeviceMemory memory() const { return m_memory; }
    VkImageView view() const { return m_view; }
    VkSampler sampler() const { return m_sampler; }

protected:
    void createSampler(const Device& device,
        VkFilter filter,
        VkSamplerAddressMode addressMode);

    void generateMipmaps(const Device& device,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VkFormat format,
        int32_t texWidth,
        int32_t texHeight);

    uint32_t m_mipLevels = 1;
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkImageView m_view = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
};


