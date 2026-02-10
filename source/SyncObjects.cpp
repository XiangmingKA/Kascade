#include "SyncObjects.h"
#include <stdexcept>

void SyncObjects::create(const Device& device, size_t maxFramesInFlight)
{
    m_imageAvailableSemaphores.resize(maxFramesInFlight);
    m_renderFinishedSemaphores.resize(maxFramesInFlight);
    m_inFlightFences.resize(maxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < maxFramesInFlight; i++) {
        if (vkCreateSemaphore(device.get(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device.get(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device.get(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void SyncObjects::destroy(const Device& device)
{
    size_t framesInFlight = m_imageAvailableSemaphores.size();
    for (size_t i = 0; i < framesInFlight; i++) {
        vkDestroySemaphore(device.get(), m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device.get(), m_imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device.get(), m_inFlightFences[i], nullptr);
    }
}
