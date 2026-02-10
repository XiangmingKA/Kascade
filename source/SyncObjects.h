#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "Device.h"

class SyncObjects
{
	public:
	void create(const Device& device, size_t maxFramesInFlight);
	void destroy(const Device& device);
	VkSemaphore getImageAvailableSemaphore(size_t frame) const { return m_imageAvailableSemaphores[frame]; }
	VkSemaphore getRenderFinishedSemaphore(size_t frame) const { return m_renderFinishedSemaphores[frame]; }
	VkFence& getInFlightFence(size_t frame) { return m_inFlightFences[frame]; }
private:
	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;
};

