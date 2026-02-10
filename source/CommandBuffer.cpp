#include "CommandBuffer.h"
#include <stdexcept>
#include <array>

void CommandPool::create(const Device& device, VkCommandPoolCreateFlags flags, VkSurfaceKHR surface)
{
    QueueFamilyIndices queueFamilyIndices = PhysicalDevice::findQueueFamilies(device.physical(), surface);
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = flags;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    if (vkCreateCommandPool(device.get(), &poolInfo, nullptr, &m_pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void CommandPool::destroy(const Device& device)
{
    if (m_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device.get(), m_pool, nullptr);
        m_pool = VK_NULL_HANDLE;
    }
}

void CommandBuffer::create(const Device& device, VkCommandPool commandPool, uint32_t count)
{
    m_commandPool = commandPool;
    m_commandBuffers.resize(count);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
    if (vkAllocateCommandBuffers(device.get(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void CommandBuffer::record(uint32_t imageIndex, uint32_t currentFrame, VkRenderPass renderPass, Swapchain swapchain,
    Framebuffer framebuffer, GraphicsPipeline graphicsPipeline, VertexBuffer vertexBuffer, IndexBuffer indexBuffer,
    DescriptorSet descriptorSets)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;
    if (vkBeginCommandBuffer(m_commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer.get()[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapchain.extent();
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.22f, 0.22f, 0.22f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(m_commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(m_commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.get());
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain.extent().width);
    viewport.height = static_cast<float>(swapchain.extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_commandBuffers[currentFrame], 0, 1, &viewport);
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain.extent();
    vkCmdSetScissor(m_commandBuffers[currentFrame], 0, 1, &scissor);
    VkBuffer vertexBuffers[] = { vertexBuffer.get() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(m_commandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(m_commandBuffers[currentFrame], indexBuffer.get(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(m_commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.getLayout(),
        0, 1, &descriptorSets[currentFrame], 0, nullptr);
    vkCmdDrawIndexed(m_commandBuffers[currentFrame], static_cast<uint32_t>(indexBuffer.size() / 4), 1, 0, 0, 0);
    vkCmdEndRenderPass(m_commandBuffers[currentFrame]);
    if (vkEndCommandBuffer(m_commandBuffers[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void CommandBuffer::reset(uint32_t index)
{
    vkResetCommandBuffer(m_commandBuffers[index], 0);
}

void CommandBuffer::destroy(const Device& device)
{
    if (!m_commandBuffers.empty()) {
        vkFreeCommandBuffers(device.get(), m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
        m_commandBuffers.clear();
    }
    m_commandPool = VK_NULL_HANDLE;
}
