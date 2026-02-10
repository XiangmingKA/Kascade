#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <GLFW/glfw3.h>

extern const std::vector<const char*> kDeviceExtensions;

extern const std::vector<const char*> kValidationLayers;

class Instance {
public:
    void createInstance(const std::string& appName, bool enableValidationLayers);
    void setupDebugMessenger(bool enableValidationLayers);
    void destroy();
    VkInstance get() const { return m_instance; }
    VkDebugUtilsMessengerEXT getDebugMessenger() const { return m_debugMessenger; }
private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
};
