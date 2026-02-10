#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include "Device.h"
#include "Texture.h"

class Cubemap : public Texture {
public:
    Cubemap() = default;
    ~Cubemap() override = default;

    void createFromFaces(const Device& device,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        void* faces[6],
        uint32_t width,
        uint32_t height,
        VkFormat format,
        bool generateMipmapsEnabled,
        VkFilter samplerFilter = VK_FILTER_LINEAR);

private:
    void createImageCube(const Device& device,
        uint32_t width, uint32_t height,
        uint32_t mipLevels,
        VkFormat format,
        VkImageUsageFlags usage);

    void transitionLayoutAll(const Device& device,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VkFormat format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout);

    void generateMipmapsCube(const Device& device,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VkFormat format,
        int32_t width, int32_t height);

    void createCubemapSampler(const Device& device, VkFilter filter);
};
