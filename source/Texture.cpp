// ============================================================================
// Texture.cpp - 2D Texture Implementation (Base class for Cubemap)
// ============================================================================

#include "Texture.h"
#include "Device.h"
#include "Utilities.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <cstring>

// ============================================================================
// Helper Functions
// ============================================================================

void transitionImageLayout(const Device& device, VkCommandPool commandPool, VkQueue graphicsQueue, 
    VkImage image, VkFormat format, VkImageLayout oldLayout, 
    VkImageLayout newLayout, uint32_t mipLevels, VkImageAspectFlagBits aspectMask) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }
    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
    endSingleTimeCommands(device, commandBuffer, commandPool, graphicsQueue);
}

// ============================================================================
// Texture Creation
// ============================================================================

void Texture::createFromPixels(const Device& device,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    const void* pixels,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    bool generateMipmapsEnabled,
    VkFilter samplerFilter,
    VkSamplerAddressMode addressMode)
{
    m_mipLevels = generateMipmapsEnabled
        ? (uint32_t)std::floor(std::log2(std::max(width, height))) + 1
        : 1u;
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    {
        VkBufferCreateInfo bufferCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferCreateInfo.size = imageSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device.get(), &bufferCreateInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
            throw std::runtime_error("Texture: create staging buffer failed");
        VkMemoryRequirements memoryRequirements{};
        vkGetBufferMemoryRequirements(device.get(), stagingBuffer, &memoryRequirements);
        VkPhysicalDeviceMemoryProperties memoryProperties{};
        vkGetPhysicalDeviceMemoryProperties(device.physical(), &memoryProperties);
        uint32_t typeIndex = UINT32_MAX;
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
            if ((memoryRequirements.memoryTypeBits & (1u << i)) &&
                (memoryProperties.memoryTypes[i].propertyFlags &
                    (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                typeIndex = i; break;
            }
        }
        if (typeIndex == UINT32_MAX) throw std::runtime_error("Texture: no memory type for staging");
        VkMemoryAllocateInfo memoryAllocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = typeIndex;
        if (vkAllocateMemory(device.get(), &memoryAllocateInfo, nullptr, &stagingMemory) != VK_SUCCESS)
            throw std::runtime_error("Texture: alloc staging memory failed");
        vkBindBufferMemory(device.get(), stagingBuffer, stagingMemory, 0);
        void* data = nullptr;
        vkMapMemory(device.get(), stagingMemory, 0, imageSize, 0, &data);
        std::memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device.get(), stagingMemory);
    }
    createImage(device, width, height, m_mipLevels,
        VK_SAMPLE_COUNT_1_BIT,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        (generateMipmapsEnabled ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_image, m_memory);
    {
        transitionImageLayout(device, commandPool, graphicsQueue,
            m_image, format,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            m_mipLevels,
            VK_IMAGE_ASPECT_COLOR_BIT);
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);
        VkBufferImageCopy copy{};
        copy.bufferOffset = 0;
        copy.bufferRowLength = 0;
        copy.bufferImageHeight = 0;
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.mipLevel = 0;
        copy.imageSubresource.baseArrayLayer = 0;
        copy.imageSubresource.layerCount = 1;
        copy.imageOffset = { 0, 0, 0 };
        copy.imageExtent = { width, height, 1 };
        vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, m_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
        endSingleTimeCommands(device, commandBuffer, commandPool, graphicsQueue);
    }
    if (generateMipmapsEnabled) {
        generateMipmaps(device, commandPool, graphicsQueue, format, (int32_t)width, (int32_t)height);
    }
    else {
        transitionImageLayout(device, commandPool, graphicsQueue,
            m_image, format,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            m_mipLevels,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }
    m_view = createImageView(device, m_image, format, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels);
    createSampler(device, samplerFilter, addressMode);
    vkDestroyBuffer(device.get(), stagingBuffer, nullptr);
    vkFreeMemory(device.get(), stagingMemory, nullptr);
}

// ============================================================================
// Protected Helper Methods
// ============================================================================

void Texture::createSampler(const Device& device,
    VkFilter filter,
    VkSamplerAddressMode addressMode)
{
    VkSamplerCreateInfo samplerCreateInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerCreateInfo.magFilter = filter;
    samplerCreateInfo.minFilter = filter;
    samplerCreateInfo.addressModeU = addressMode;
    samplerCreateInfo.addressModeV = addressMode;
    samplerCreateInfo.addressModeW = addressMode;
    samplerCreateInfo.anisotropyEnable = VK_TRUE;
    samplerCreateInfo.maxAnisotropy = 16.0f;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = static_cast<float>(m_mipLevels);
    samplerCreateInfo.mipLodBias = 0.0f;
    if (vkCreateSampler(device.get(), &samplerCreateInfo, nullptr, &m_sampler) != VK_SUCCESS)
        throw std::runtime_error("Texture: create sampler failed");
}

void Texture::generateMipmaps(const Device& device,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    VkFormat format,
    int32_t texWidth,
    int32_t texHeight)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);
    VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.image = m_image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;
    for (uint32_t i = 1; i < m_mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        int32_t nextWidth = std::max(mipWidth / 2, 1);
        int32_t nextHeight = std::max(mipHeight / 2, 1);
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { nextWidth, nextHeight, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        vkCmdBlitImage(commandBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit, VK_FILTER_LINEAR);
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
        mipWidth = nextWidth;
        mipHeight = nextHeight;
    }
    barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
    endSingleTimeCommands(device, commandBuffer, commandPool, graphicsQueue);
}

// ============================================================================
// Cleanup
// ============================================================================

void Texture::destroy(const Device& device)
{
    if (m_sampler) { vkDestroySampler(device.get(), m_sampler, nullptr); m_sampler = VK_NULL_HANDLE; }
    if (m_view) { vkDestroyImageView(device.get(), m_view, nullptr);  m_view = VK_NULL_HANDLE; }
    if (m_image) { vkDestroyImage(device.get(), m_image, nullptr);     m_image = VK_NULL_HANDLE; }
    if (m_memory) { vkFreeMemory(device.get(), m_memory, nullptr);      m_memory = VK_NULL_HANDLE; }
    m_mipLevels = 1;
}
