#pragma once

#include <cstdint>
#include <vector>

#include "Instance.h"
#include "Surface.h"
#include "PhysicalDevice.h"
#include "Device.h"
#include "Swapchain.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "Descriptor.h"
#include "Framebuffer.h"
#include "CommandBuffer.h"
#include "SyncObjects.h"
#include "Vertex.h"
#include "VertexIndexBuffers.h"
#include "UniformBuffers.h"
#include "Texture.h"
#include "Cubemap.h"
#include "GltfModel.h"
#include "GltfDescriptors.h"

// Forward declaration only, glfw3.h is not included (to reduce compilation overhead)
struct GLFWwindow;

class Application final {
public:
    void run();

private:
    // ---- Constants ----
    static constexpr const char* kApplicationName = "VULKAN";
    static constexpr uint32_t kWidth = 1500;
    static constexpr uint32_t kHeight = 1200;
    static constexpr int kMaxFramesInFlight = 2;
#ifndef NDEBUG
    static constexpr bool kEnableValidationLayers = true;
#else
    static constexpr bool kEnableValidationLayers = false;
#endif

    // ---- Window & Callback ----
    GLFWwindow* m_window = nullptr;
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    // ---- Vulkan Objects ----
    Instance m_instance;
    Surface m_surface;
    PhysicalDevice m_physicalDevice;
    Device m_device;
    Swapchain m_swapchain;
    RenderPass m_renderPass;
    PipelineLayoutRAII m_pipelineLayout;
    GraphicsPipeline m_graphicsPipeline;
    DescriptorSetLayoutRAII m_descriptorSetLayout;
    Framebuffer m_framebuffer;
    CommandPool m_commandPool;
    CommandBuffer m_commandBuffers;

    SyncObjects m_syncObjects;
    VertexBuffer m_vertexBuf;
    IndexBuffer m_indexBuf;
    UniformBufferSet m_uboSet;

    DescriptorPoolRAII m_descriptorPool;
    DescriptorSet m_descriptorSets;

    Texture m_baseTexture;
    Texture m_normalTexture;
    Texture m_metalRoughnessTexture;
    Texture m_aoTexture;
    Texture m_emissiveTexture;
	Cubemap m_cubemap;
    bool m_framebufferResized = false;
    uint32_t m_currentFrame = 0;

    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    // ---- glTF Rendering System ----
    GltfModel m_gltfModel;
    GltfDescriptorSetLayouts m_gltfDescriptorLayouts;
    GltfDescriptorPool m_gltfDescriptorPool;
    GltfDescriptorSets m_gltfDescriptorSets;
    PipelineLayoutRAII m_gltfPipelineLayout;
    VkPipeline m_gltfPipeline = VK_NULL_HANDLE;

    // ---- Lifecycle ----
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    void drawFrame();

    // ---- Debug Utils destruction helper (for direct calls within the class) ----
    static void destroyDebugUtilsMessengerEXT(VkInstance instance,
        VkDebugUtilsMessengerEXT messenger,
        const VkAllocationCallbacks* allocator);

    // ---- Initialization Helpers ----
    void initVulkanCore();
    void initRenderResources();
    void initTextures();
    void initGeometry();
    void initDescriptors();

    // ---- glTF Initialization ----
    void initGltf();
    void loadGltfModel();
    void createGltfPipeline();
    void updateGltfDescriptors();

    // ---- Model & Texture Loading ----
    void loadModel();
    void loadCubemap();
    void computeTangents();
    Texture loadTexture(const std::string& path, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
                        VkFilter filter = VK_FILTER_LINEAR,
                        VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

    // ---- Descriptor Helpers ----
    VkDescriptorImageInfo createDescriptorImageInfo(const Texture& texture);
    VkWriteDescriptorSet createDescriptorWrite(VkDescriptorSet dstSet, uint32_t binding,
                                                VkDescriptorType type, const void* pInfo);

    // ---- Static Utilities ----
    static std::vector<char> readFile(const std::string& filename);
    static VkShaderModule createShaderModule(Device device, const std::vector<char>& code);

    void updateUniformBuffer(uint32_t currentImage);

	// ---- Runtime Data ----
	glm::vec3 origin = { 0.0f, 0.0f, 0.0f };
    glm::vec3 viewPos = { 0.8f, 0.8f, 0.6f };  // Moved camera closer
    glm::vec3 viewDir = { 0.0f, 0.0f, 0.0f };
};

