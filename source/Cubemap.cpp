// ============================================================================
// Cubemap.cpp - Cubemap Texture Implementation (Inherits from Texture)
// ============================================================================

#include "Cubemap.h"
#include "Device.h"
#include "Utilities.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <cstring>

// ============================================================================
// Helper Functions
// ============================================================================

static uint32_t bytesPerPixel(VkFormat fmt) {
    switch (fmt) {
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB: return 4;
    case VK_FORMAT_R16G16B16A16_SFLOAT: return 8; // 16-bit float * 4
    default: throw std::runtime_error("Cubemap: unsupported format in bytesPerPixel");
    }
}

// ============================================================================
// Cubemap Creation
// ============================================================================

void Cubemap::createFromFaces(const Device& device,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    void* faces[6],
    uint32_t width,
    uint32_t height,
    VkFormat format,
    bool generateMipmapsEnabled,
    VkFilter samplerFilter)
{
    if (!faces || !faces[0]) throw std::runtime_error("Cubemap: faces null");

    m_mipLevels = generateMipmapsEnabled
        ? (uint32_t)std::floor(std::log2(std::max(width, height))) + 1
        : 1u;

    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (generateMipmapsEnabled) usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    createImageCube(device, width, height, m_mipLevels, format, usage);

    transitionLayoutAll(device, commandPool, graphicsQueue,
        format, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    const VkDeviceSize faceSize = (VkDeviceSize)width * height * bytesPerPixel(format);

    for (uint32_t f = 0; f < 6; ++f) {
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

        // staging buffer
        {
            VkBufferCreateInfo bi{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bi.size = faceSize;
            bi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            if (vkCreateBuffer(device.get(), &bi, nullptr, &stagingBuffer) != VK_SUCCESS)
                throw std::runtime_error("Cubemap: create staging buffer failed");

            VkMemoryRequirements memReq{};
            vkGetBufferMemoryRequirements(device.get(), stagingBuffer, &memReq);

            VkPhysicalDeviceMemoryProperties memProp{};
            vkGetPhysicalDeviceMemoryProperties(device.physical(), &memProp);

            uint32_t typeIndex = UINT32_MAX;
            for (uint32_t i = 0; i < memProp.memoryTypeCount; ++i) {
                if ((memReq.memoryTypeBits & (1u << i)) &&
                    (memProp.memoryTypes[i].propertyFlags &
                        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
                    (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                    typeIndex = i; break;
                }
            }
            if (typeIndex == UINT32_MAX) throw std::runtime_error("Cubemap: no staging mem type");

            VkMemoryAllocateInfo mai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            mai.allocationSize = memReq.size;
            mai.memoryTypeIndex = typeIndex;
            if (vkAllocateMemory(device.get(), &mai, nullptr, &stagingMemory) != VK_SUCCESS)
                throw std::runtime_error("Cubemap: alloc staging memory failed");

            vkBindBufferMemory(device.get(), stagingBuffer, stagingMemory, 0);

            void* map = nullptr;
            vkMapMemory(device.get(), stagingMemory, 0, faceSize, 0, &map);
            std::memcpy(map, faces[f], (size_t)faceSize);
            vkUnmapMemory(device.get(), stagingMemory);
        }

        VkCommandBuffer cmd = beginSingleTimeCommands(device, commandPool);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = f; 
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(cmd, stagingBuffer, m_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(device, cmd, commandPool, graphicsQueue);

        vkDestroyBuffer(device.get(), stagingBuffer, nullptr);
        vkFreeMemory(device.get(), stagingMemory, nullptr);
    }

    if (generateMipmapsEnabled) {
        generateMipmapsCube(device, commandPool, graphicsQueue, format,
            (int32_t)width, (int32_t)height);
    }
    else {
        transitionLayoutAll(device, commandPool, graphicsQueue, format,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    VkImageViewCreateInfo vi{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    vi.image = m_image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    vi.format = format;
    vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vi.subresourceRange.baseMipLevel = 0;
    vi.subresourceRange.levelCount = m_mipLevels;
    vi.subresourceRange.baseArrayLayer = 0;
    vi.subresourceRange.layerCount = 6;
    if (vkCreateImageView(device.get(), &vi, nullptr, &m_view) != VK_SUCCESS)
        throw std::runtime_error("Cubemap: create image view failed");

    createCubemapSampler(device, samplerFilter);
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void Cubemap::createImageCube(const Device& device,
    uint32_t width, uint32_t height,
    uint32_t mipLevels,
    VkFormat format,
    VkImageUsageFlags usage)
{
    VkImageCreateInfo ci{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    ci.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = format;
    ci.extent = { width, height, 1 };
    ci.mipLevels = mipLevels;
    ci.arrayLayers = 6;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device.get(), &ci, nullptr, &m_image) != VK_SUCCESS)
        throw std::runtime_error("Cubemap: create image failed");

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(device.get(), m_image, &req);

    VkPhysicalDeviceMemoryProperties mp{};
    vkGetPhysicalDeviceMemoryProperties(device.physical(), &mp);

    uint32_t typeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < mp.memoryTypeCount; ++i) {
        if ((req.memoryTypeBits & (1u << i)) &&
            (mp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            typeIndex = i; break;
        }
    }
    if (typeIndex == UINT32_MAX) throw std::runtime_error("Cubemap: no device local memory type");

    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = typeIndex;

    if (vkAllocateMemory(device.get(), &ai, nullptr, &m_memory) != VK_SUCCESS)
        throw std::runtime_error("Cubemap: alloc memory failed");

    vkBindImageMemory(device.get(), m_image, m_memory, 0);
}

void Cubemap::transitionLayoutAll(const Device& device,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    VkFormat /*format*/,
    VkImageLayout oldLayout,
    VkImageLayout newLayout)
{
    VkCommandBuffer cmd = beginSingleTimeCommands(device, commandPool);

    VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.image = m_image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 6;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    endSingleTimeCommands(device, cmd, commandPool, graphicsQueue);
}

void Cubemap::generateMipmapsCube(const Device& device,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    VkFormat format,
    int32_t texWidth,
    int32_t texHeight)
{
    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(device.physical(), format, &props);
    if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == 0) {
        throw std::runtime_error("Cubemap: format does not support linear blit for mipmaps");
    }

    VkCommandBuffer cmd = beginSingleTimeCommands(device, commandPool);

    VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.image = m_image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    for (uint32_t layer = 0; layer < 6; ++layer) {
        int32_t mipW = texWidth;
        int32_t mipH = texHeight;

        for (uint32_t level = 1; level < m_mipLevels; ++level) {
            barrier.subresourceRange.baseMipLevel = level - 1;
            barrier.subresourceRange.baseArrayLayer = layer;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrier);

            int32_t nextW = std::max(mipW / 2, 1);
            int32_t nextH = std::max(mipH / 2, 1);

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipW, mipH, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = level - 1;
            blit.srcSubresource.baseArrayLayer = layer;
            blit.srcSubresource.layerCount = 1;

            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { nextW, nextH, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = level;
            blit.dstSubresource.baseArrayLayer = layer;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(cmd,
                m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrier);

            mipW = nextW;
            mipH = nextH;
        }

        barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    endSingleTimeCommands(device, cmd, commandPool, graphicsQueue);
}

void Cubemap::createCubemapSampler(const Device& device, VkFilter filter)
{
    VkSamplerCreateInfo si{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    si.magFilter = filter;
    si.minFilter = filter;
    si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    si.anisotropyEnable = VK_FALSE;
    si.maxAnisotropy = 1.0f;
    si.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    si.unnormalizedCoordinates = VK_FALSE;
    si.compareEnable = VK_FALSE;
    si.minLod = 0.0f;
    si.maxLod = float(m_mipLevels - 1);
    si.mipLodBias = 0.0f;

    if (vkCreateSampler(device.get(), &si, nullptr, &m_sampler) != VK_SUCCESS)
        throw std::runtime_error("Cubemap: create sampler failed");
}
