#include "Framebuffer.h"
#include "Utilities.h"
#include <array>
#include <stdexcept>

void Framebuffer::createColorResources(const Device& device, VkFormat colorFormat, VkSampleCountFlagBits msaa, VkExtent2D extent)
{
    createImage(device, extent.width, extent.height, 1, msaa, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_colorImage, m_colorImageMemory);
    m_colorImageView = createImageView(device, m_colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void Framebuffer::createDepthResources(const Device& device, VkFormat depthFormat, VkSampleCountFlagBits msaa, VkExtent2D extent)
{
    createImage(device, extent.width, extent.height, 1, msaa,
        depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_depthImage, m_depthImageMemory);
    m_depthImageView = createImageView(device, m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Framebuffer::destroyColorResources(const Device& device)
{
    vkDestroyImageView(device.get(), m_colorImageView, nullptr);
    vkDestroyImage(device.get(), m_colorImage, nullptr);
    vkFreeMemory(device.get(), m_colorImageMemory, nullptr);
}

void Framebuffer::destroyDepthResources(const Device& device)
{
    vkDestroyImageView(device.get(), m_depthImageView, nullptr);
    vkDestroyImage(device.get(), m_depthImage, nullptr);
    vkFreeMemory(device.get(), m_depthImageMemory, nullptr);
}

void Framebuffer::create(const Device& device, const Swapchain& swapchain, const RenderPass& renderPass)
{
    createColorResources(device, swapchain.imageFormat(), swapchain.msaa(), swapchain.extent());

    VkFormat depthFormat = findDepthFormat(device);
    createDepthResources(device, depthFormat, swapchain.msaa(), swapchain.extent());

    m_framebuffers.resize(swapchain.imageViews().size());
    for (size_t i = 0; i < swapchain.imageViews().size(); i++) {
        std::array<VkImageView, 3> attachments = {
            m_colorImageView,
            m_depthImageView,
            swapchain.imageViews()[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass.get();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchain.extent().width;
        framebufferInfo.height = swapchain.extent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device.get(), &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void Framebuffer::destroy(const Device& device)
{
    destroyColorResources(device);
    destroyDepthResources(device);

    for (auto framebuffer : m_framebuffers) {
        vkDestroyFramebuffer(device.get(), framebuffer, nullptr);
    }

    m_framebuffers.clear();
}
