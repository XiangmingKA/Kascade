// ============================================================================
// Application.cpp - Main Vulkan Application Implementation
// ============================================================================

#include "Application.h"
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include <array>
#include <unordered_map>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
// NOTE: STB_IMAGE_IMPLEMENTATION is defined in GltfModel.cpp via tinygltf
#include <stb_image.h>
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#undef max

// ============================================================================
// Constants & Configuration
// ============================================================================

const std::string kModelPath = "models/DamagedHelmet.obj";
const std::string kBaseTexturePath = "textures/DamagedHelmet_Albedo.jpg";
const std::string kNormalTexturePath = "textures/DamagedHelmet_Normal.jpg";
const std::string kMetalRoughnessTexturePath = "textures/DamagedHelmet_MetalRoughness.jpg";
const std::string kAoTexturePath = "textures/DamagedHelmet_AO.jpg";
const std::string kEmissiveTexturePath = "textures/DamagedHelmet_Emissive.jpg";
const std::string kCubemapPath = "textures/Cubemaps/CloudySky/";

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 normalMat;
    glm::vec4 cameraPosition;
    glm::vec4 viewDirection;
    glm::vec4 sunDirection;
    glm::vec4 sunColor;
    glm::vec4 ambientLight;
};

// ============================================================================
// Callback & Debug Functions
// ============================================================================

void Application::destroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT messenger,
    const VkAllocationCallbacks* allocator)
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func) func(instance, messenger, allocator);
}

void Application::framebufferResizeCallback(GLFWwindow* window, int, int)
{
    auto app = reinterpret_cast<Application*>(
        glfwGetWindowUserPointer(window));
    if (app) app->m_framebufferResized = true;
}

// ============================================================================
// Application Lifecycle
// ============================================================================

void Application::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void Application::initWindow() {
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(kWidth, kHeight, kApplicationName, nullptr, nullptr);
    if (!m_window) throw std::runtime_error("glfwCreateWindow failed");

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

// ============================================================================
// Static Utility Methods
// ============================================================================

std::vector<char> Application::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

VkShaderModule Application::createShaderModule(Device device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device.get(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }

    return shaderModule;
}

// ============================================================================
// Texture & Descriptor Helper Methods
// ============================================================================

Texture Application::loadTexture(const std::string& path, VkFormat format,
                                  VkFilter filter, VkSamplerAddressMode addressMode) {
    int width, height, channels;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error("Failed to load texture: " + path);
    }

    Texture texture;
    texture.createFromPixels(m_device, m_commandPool.get(), m_device.graphicsQ(),
                             pixels, width, height, format, true, filter, addressMode);
    stbi_image_free(pixels);

    return texture;
}

VkDescriptorImageInfo Application::createDescriptorImageInfo(const Texture& texture) {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture.view();
    imageInfo.sampler = texture.sampler();
    return imageInfo;
}

VkWriteDescriptorSet Application::createDescriptorWrite(VkDescriptorSet dstSet, uint32_t binding,
                                                         VkDescriptorType type, const void* pInfo) {
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = dstSet;
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorType = type;
    write.descriptorCount = 1;

    if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
        write.pBufferInfo = static_cast<const VkDescriptorBufferInfo*>(pInfo);
    } else if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
        write.pImageInfo = static_cast<const VkDescriptorImageInfo*>(pInfo);
    }

    return write;
}

// ============================================================================
// Model Loading
// ============================================================================

void Application::loadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err, warn;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, kModelPath.c_str())) {
        throw std::runtime_error(err.empty() ? "LoadObj failed" : err);
    }
    if (!warn.empty()) { std::cerr << "TinyObjLoader warn: " << warn << "\n"; }

    // Build indexed mesh (unique by pos+normal+uv(+color)); tangent is filled later
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    m_vertices.clear();
    m_indices.clear();

    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            size_t fv = static_cast<size_t>(shape.mesh.num_face_vertices[f]);

            // Tangent calculation assumes triangles. Ensure your OBJ is triangulated.
            if (fv != 3) {
                // You can triangulate offline or via ObjReaderConfig.triangulate.
                // Here we skip non-tri faces to be safe.
                index_offset += fv;
                continue;
            }

            for (size_t v = 0; v < fv; ++v) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                Vertex vert{};
                // position
                vert.pos = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                };
                // normal (required for tangent frame; if missing, you could compute face normals first)
                if (idx.normal_index >= 0) {
                    vert.normals = {
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]
                    };
                }
                else {
                    vert.normals = glm::vec3(0, 0, 1); // fallback; better: compute per-face normals first
                }
                // texcoord (required for tangents)
                if (idx.texcoord_index >= 0) {
                    vert.texCoord = {
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * idx.texcoord_index + 1] // flip V like your original
                    };
                }
                else {
                    vert.texCoord = glm::vec2(0.0f); // no UVs -> tangents will be arbitrary
                }
                // color (optional)
                if (!attrib.colors.empty() && idx.vertex_index >= 0) {
                    vert.color = {
                        attrib.colors[3 * idx.vertex_index + 0],
                        attrib.colors[3 * idx.vertex_index + 1],
                        attrib.colors[3 * idx.vertex_index + 2]
                    };
                }
                else {
                    vert.color = glm::vec3(1.0f);
                }
                // placeholder tangent
                vert.tangent = glm::vec4(0, 0, 0, 1);

                if (uniqueVertices.count(vert) == 0) {
                    uniqueVertices[vert] = static_cast<uint32_t>(m_vertices.size());
                    m_vertices.push_back(vert);
                }
                m_indices.push_back(uniqueVertices[vert]);
            }

            index_offset += fv;
        }
    }

    computeTangents();
}

void Application::computeTangents() {
    const size_t V = m_vertices.size();
    if (V == 0 || m_indices.size() < 3) return;

    std::vector<glm::vec3> tan1(V, glm::vec3(0.0f));
    std::vector<glm::vec3> tan2(V, glm::vec3(0.0f));

    auto len2 = [](const glm::vec3& v) { return glm::dot(v, v); };

    // Accumulate tangent and bitangent for each triangle
    for (size_t i = 0; i + 2 < m_indices.size(); i += 3) {
        uint32_t i0 = m_indices[i + 0];
        uint32_t i1 = m_indices[i + 1];
        uint32_t i2 = m_indices[i + 2];

        const glm::vec3& p0 = m_vertices[i0].pos;
        const glm::vec3& p1 = m_vertices[i1].pos;
        const glm::vec3& p2 = m_vertices[i2].pos;

        const glm::vec2& uv0 = m_vertices[i0].texCoord;
        const glm::vec2& uv1 = m_vertices[i1].texCoord;
        const glm::vec2& uv2 = m_vertices[i2].texCoord;

        glm::vec3 dp1 = p1 - p0;
        glm::vec3 dp2 = p2 - p0;
        glm::vec2 duv1 = uv1 - uv0;
        glm::vec2 duv2 = uv2 - uv0;

        float denom = duv1.x * duv2.y - duv2.x * duv1.y;
        if (std::abs(denom) < 1e-20f) {
            continue; // Degenerate UVs: skip this triangle
        }
        float r = 1.0f / denom;

        glm::vec3 T = (dp1 * duv2.y - dp2 * duv1.y) * r;
        glm::vec3 B = (dp2 * duv1.x - dp1 * duv2.x) * r;

        tan1[i0] += T; tan1[i1] += T; tan1[i2] += T;
        tan2[i0] += B; tan2[i1] += B; tan2[i2] += B;
    }

    // Orthogonalize and normalize tangents per-vertex
    for (size_t i = 0; i < V; ++i) {
        glm::vec3 N = glm::normalize(m_vertices[i].normals);
        glm::vec3 T = tan1[i];

        // Fallback if degenerate
        if (len2(T) < 1e-20f) {
            glm::vec3 a = (std::abs(N.z) < 0.999f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
            T = glm::normalize(glm::cross(a, N));
        }

        // Gram-Schmidt: orthogonalize T against N
        T = glm::normalize(T - N * glm::dot(N, T));

        glm::vec3 B = tan2[i];
        // Compute handedness: if (N x T) roughly points along B -> +1, else -1
        float w = (glm::dot(glm::cross(N, T), B) < 0.0f) ? -1.0f : 1.0f;

        m_vertices[i].normals = N;
        m_vertices[i].tangent = glm::vec4(T, w);
    }
}

void Application::loadCubemap() {
    // +X, -X, +Y, -Y, +Z, -Z
    const std::string kFaces[6] = {
      kCubemapPath + "px.png", kCubemapPath + "nx.png", 
      kCubemapPath + "py.png", kCubemapPath + "ny.png", 
      kCubemapPath + "pz.png", kCubemapPath + "nz.png"
    };

    int w = 0, h = 0, ch = 0;
    stbi_uc* faces[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    for (int f = 0; f < 6; ++f) {
        int tw, th, tc;
        faces[f] = stbi_load(kFaces[f].c_str(), &tw, &th, &tc, 4); // 强制 RGBA
        if (!faces[f]) throw std::runtime_error("stbi_load failed: " + kFaces[f]);
        if (f == 0) { w = tw; h = th; ch = 4; }
        else if (tw != w || th != h) throw std::runtime_error("cubemap face size mismatch");
    }

    m_cubemap.createFromFaces(
        m_device, m_commandPool.get(), m_device.graphicsQ(),
        reinterpret_cast<void**>(faces),
        static_cast<uint32_t>(w),
        static_cast<uint32_t>(h),
        VK_FORMAT_R8G8B8A8_SRGB, /* generateMip = */ true,
        VK_FILTER_LINEAR
    );

    for (int f = 0; f < 6; ++f) {
        stbi_image_free(faces[f]);
    }
}

// ============================================================================
// Vulkan Initialization
// ============================================================================

void Application::initVulkan() {
    initVulkanCore();
    initRenderResources();
    initTextures();
    initGeometry();
    initDescriptors();
    initGltf();  // Initialize and load glTF model
}

void Application::initVulkanCore() {
    m_instance.createInstance(kApplicationName, kEnableValidationLayers);
    m_instance.setupDebugMessenger(kEnableValidationLayers);
    m_surface.create(m_instance.get(), m_window);
    m_physicalDevice.pick(m_instance.get(), m_surface.get());
    m_device.create(m_physicalDevice, kEnableValidationLayers);
}

void Application::initRenderResources() {
    m_swapchain.create(m_device, m_surface.get(), m_window);
    m_renderPass.create(m_device, m_swapchain.imageFormat(), m_swapchain.depthFormat(), m_msaaSamples);
    m_descriptorSetLayout.create(m_device);

    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(m_device, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(m_device, fragShaderCode);

    m_pipelineLayout.create(m_device, {}, {});
    m_graphicsPipeline.create(m_device, m_renderPass.get(), m_pipelineLayout.get(),
                              m_swapchain.extent(), m_msaaSamples, m_descriptorSetLayout.get(),
                              vertShaderModule, fragShaderModule);

    m_commandPool.create(m_device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_surface.get());
    m_framebuffer.create(m_device, m_swapchain, m_renderPass);
    m_commandBuffers.create(m_device, m_commandPool.get(), kMaxFramesInFlight);
    m_syncObjects.create(m_device, kMaxFramesInFlight);
}

void Application::initTextures() {
    m_baseTexture = loadTexture(kBaseTexturePath);
    m_normalTexture = loadTexture(kNormalTexturePath);
    m_metalRoughnessTexture = loadTexture(kMetalRoughnessTexturePath);
    m_aoTexture = loadTexture(kAoTexturePath);
    m_emissiveTexture = loadTexture(kEmissiveTexturePath);
    loadCubemap();
}

void Application::initGeometry() {
    loadModel();
    VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();
    m_vertexBuf.createFrom(m_device, m_commandPool.get(), m_device.graphicsQ(), m_vertices.data(), bufferSize);
    m_indexBuf.createFromVector(m_device, m_commandPool.get(), m_device.graphicsQ(), m_indices);
    m_uboSet.create(m_device, kMaxFramesInFlight, sizeof(UniformBufferObject));
}

void Application::initDescriptors() {
    m_descriptorPool.create(m_device);
    m_descriptorSets.create(m_device, m_descriptorPool.get(), m_descriptorSetLayout.get());

    for (size_t i = 0; i < kMaxFramesInFlight; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uboSet.buffer(i);
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo baseImageInfo = createDescriptorImageInfo(m_baseTexture);
        VkDescriptorImageInfo normalImageInfo = createDescriptorImageInfo(m_normalTexture);
        VkDescriptorImageInfo metalRoughnessImageInfo = createDescriptorImageInfo(m_metalRoughnessTexture);
        VkDescriptorImageInfo aoImageInfo = createDescriptorImageInfo(m_aoTexture);
        VkDescriptorImageInfo emissiveImageInfo = createDescriptorImageInfo(m_emissiveTexture);
        VkDescriptorImageInfo cubemapImageInfo = createDescriptorImageInfo(m_cubemap);

        std::array<VkWriteDescriptorSet, 7> descriptorWrites{};
        descriptorWrites[0] = createDescriptorWrite(m_descriptorSets[i], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
        descriptorWrites[1] = createDescriptorWrite(m_descriptorSets[i], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &baseImageInfo);
        descriptorWrites[2] = createDescriptorWrite(m_descriptorSets[i], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &normalImageInfo);
        descriptorWrites[3] = createDescriptorWrite(m_descriptorSets[i], 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &metalRoughnessImageInfo);
        descriptorWrites[4] = createDescriptorWrite(m_descriptorSets[i], 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &aoImageInfo);
        descriptorWrites[5] = createDescriptorWrite(m_descriptorSets[i], 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &emissiveImageInfo);
        descriptorWrites[6] = createDescriptorWrite(m_descriptorSets[i], 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &cubemapImageInfo);

        vkUpdateDescriptorSets(m_device.get(), static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
}

// ============================================================================
// glTF Initialization
// ============================================================================

void Application::initGltf() {
    // Initialize glTF subsystem: descriptor layouts, pool, model loading, and pipeline
    m_gltfDescriptorLayouts.create(m_device);
    m_gltfDescriptorPool.create(m_device, 32); // Support up to 32 textures
    m_gltfDescriptorSets.allocate(m_device, m_gltfDescriptorPool.get(), m_gltfDescriptorLayouts, kMaxFramesInFlight);

    loadGltfModel();
    createGltfPipeline();
    updateGltfDescriptors();
}

void Application::loadGltfModel() {
    // Load the ToyCar test model
    m_gltfModel.loadFromFile(m_device, m_commandPool.get(), m_device.graphicsQ(),
                              "models/ABeautifulGame/glTF/ABeautifulGame.gltf");
}

void Application::createGltfPipeline() {
    // Load glTF shaders
    auto vertShaderCode = readFile("shaders/gltf_vert.spv");
    auto fragShaderCode = readFile("shaders/gltf_frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(m_device, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(m_device, fragShaderCode);

    // Define push constants for node and material indices
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(int) * 2;  // nodeIndex + materialIndex

    // Create pipeline layout with multiple descriptor sets and push constants
    m_gltfPipelineLayout.create(m_device, m_gltfDescriptorLayouts.getAllLayouts(), { pushConstantRange });

    // Create the actual graphics pipeline using GltfVertex
    VkPipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertShaderModule;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragShaderModule;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

    // Vertex input state - use GltfVertex
    auto bindingDescription = GltfVertex::getBindingDescription();
    auto attributeDescriptions = GltfVertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport state (dynamic)
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = m_msaaSamples;

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Dynamic state
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_gltfPipelineLayout.get();
    pipelineInfo.renderPass = m_renderPass.get();
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(m_device.get(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_gltfPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create glTF graphics pipeline");
    }

    vkDestroyShaderModule(m_device.get(), fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device.get(), vertShaderModule, nullptr);
}

void Application::updateGltfDescriptors() {
    // Only update if glTF model is loaded
    if (!m_gltfModel.isLoaded()) return;

    // Update descriptor sets with actual buffer and texture data
    for (uint32_t i = 0; i < kMaxFramesInFlight; i++) {
        std::vector<VkWriteDescriptorSet> writes;

        // Set 0, Binding 0: Per-frame UBO (camera, lights)
        VkDescriptorBufferInfo uboInfo{};
        uboInfo.buffer = m_uboSet.buffer(i);
        uboInfo.offset = 0;
        uboInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet uboWrite{};
        uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uboWrite.dstSet = m_gltfDescriptorSets.getPerFrameSet(i);
        uboWrite.dstBinding = 0;
        uboWrite.dstArrayElement = 0;
        uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboWrite.descriptorCount = 1;
        uboWrite.pBufferInfo = &uboInfo;
        writes.push_back(uboWrite);

        // Set 1, Binding 0: Transform storage buffer
        VkDescriptorBufferInfo transformInfo{};
        transformInfo.buffer = m_gltfModel.getTransformBuffer();
        transformInfo.offset = 0;
        transformInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet transformWrite{};
        transformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        transformWrite.dstSet = m_gltfDescriptorSets.getPerModelSet(i);
        transformWrite.dstBinding = 0;
        transformWrite.dstArrayElement = 0;
        transformWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        transformWrite.descriptorCount = 1;
        transformWrite.pBufferInfo = &transformInfo;
        writes.push_back(transformWrite);

        // Set 1, Binding 1: Material storage buffer
        VkDescriptorBufferInfo materialInfo{};
        materialInfo.buffer = m_gltfModel.getMaterialBuffer();
        materialInfo.offset = 0;
        materialInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet materialWrite{};
        materialWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        materialWrite.dstSet = m_gltfDescriptorSets.getPerModelSet(i);
        materialWrite.dstBinding = 1;
        materialWrite.dstArrayElement = 0;
        materialWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        materialWrite.descriptorCount = 1;
        materialWrite.pBufferInfo = &materialInfo;
        writes.push_back(materialWrite);

        // Set 2, Binding 0: Texture array
        const auto& textures = m_gltfModel.getTextures();
        std::vector<VkDescriptorImageInfo> imageInfos(std::max(1u, static_cast<uint32_t>(textures.size())));

        if (!textures.empty()) {
            for (size_t t = 0; t < textures.size(); ++t) {
                imageInfos[t].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfos[t].imageView = textures[t].view();
                imageInfos[t].sampler = textures[t].sampler();
            }
        } else {
            // If no textures, use a dummy texture (base texture as fallback)
            imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[0].imageView = m_baseTexture.view();
            imageInfos[0].sampler = m_baseTexture.sampler();
        }

        VkWriteDescriptorSet texturesWrite{};
        texturesWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        texturesWrite.dstSet = m_gltfDescriptorSets.getTexturesSet(i);
        texturesWrite.dstBinding = 0;
        texturesWrite.dstArrayElement = 0;
        texturesWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texturesWrite.descriptorCount = static_cast<uint32_t>(imageInfos.size());
        texturesWrite.pImageInfo = imageInfos.data();
        writes.push_back(texturesWrite);

        // Set 3, Binding 0: Environment cubemap
        VkDescriptorImageInfo cubemapInfo{};
        cubemapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        cubemapInfo.imageView = m_cubemap.view();
        cubemapInfo.sampler = m_cubemap.sampler();

        VkWriteDescriptorSet cubemapWrite{};
        cubemapWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        cubemapWrite.dstSet = m_gltfDescriptorSets.getEnvironmentSet(i);
        cubemapWrite.dstBinding = 0;
        cubemapWrite.dstArrayElement = 0;
        cubemapWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        cubemapWrite.descriptorCount = 1;
        cubemapWrite.pImageInfo = &cubemapInfo;
        writes.push_back(cubemapWrite);

        // Update all descriptor sets
        vkUpdateDescriptorSets(m_device.get(), static_cast<uint32_t>(writes.size()),
                               writes.data(), 0, nullptr);
    }
}

// ============================================================================
// Main Loop & Rendering
// ============================================================================

void Application::mainLoop() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(m_device.get());
}

void Application::updateUniformBuffer(uint32_t currentImage) {


	viewDir = glm::normalize(origin - viewPos);

    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));  // Rotate around Y-axis
	ubo.view = glm::lookAt(viewPos, viewPos + viewDir, glm::vec3(0.0f, 1.0f, 0.0f));  // Y-up for glTF
    ubo.proj = glm::perspective(glm::radians(45.0f), (float)kWidth / (float)kHeight, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1; // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
	ubo.normalMat = glm::transpose(glm::inverse(ubo.model));
    ubo.cameraPosition = glm::vec4(viewPos, 0.0f);
    ubo.viewDirection = glm::vec4(-viewDir, 0.0f);

	ubo.sunDirection = glm::vec4(glm::normalize(glm::vec3(-1.0f, 2.0f, 2.0f)), 0.0f);
	ubo.sunColor = glm::vec4(10.0f, 9.8f, 8.8f, 1.0f);
	ubo.ambientLight = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

    m_uboSet.updateFrame(currentImage, &ubo, sizeof(ubo));
}

void Application::drawFrame() {
    vkWaitForFences(m_device.get(), 1, &m_syncObjects.getInFlightFence(m_currentFrame), VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_device.get(), 
        m_swapchain.get(), UINT64_MAX, m_syncObjects.getImageAvailableSemaphore(m_currentFrame), VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        m_swapchain.recreate(m_device, m_surface.get(), m_window);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Only reset the fence if we are submitting work
    vkResetFences(m_device.get(), 1, &m_syncObjects.getInFlightFence(m_currentFrame));

    updateUniformBuffer(m_currentFrame);

    // Update glTF transforms if model is loaded
    if (m_gltfModel.isLoaded()) {
        m_gltfModel.updateTransforms(m_device);
    }

    m_commandBuffers.reset(m_currentFrame);

    // Manually record command buffer to include both OBJ and glTF rendering
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer");
    }

    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass.get();
    renderPassInfo.framebuffer = m_framebuffer.get()[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchain.extent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.22f, 0.22f, 0.22f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchain.extent().width);
    viewport.height = static_cast<float>(m_swapchain.extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchain.extent();
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Draw OBJ model (DISABLED - showing glTF only)
    // vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline.get());
    // VkBuffer vertexBuffers[] = {m_vertexBuf.get()};
    // VkDeviceSize offsets[] = {0};
    // vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    // vkCmdBindIndexBuffer(cmd, m_indexBuf.get(), 0, VK_INDEX_TYPE_UINT32);
    // vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline.getLayout(),
    //                         0, 1, &m_descriptorSets[m_currentFrame], 0, nullptr);
    // vkCmdDrawIndexed(cmd, static_cast<uint32_t>(m_indexBuf.size() / 4), 1, 0, 0, 0);

    // Draw glTF model
    if (m_gltfModel.isLoaded()) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gltfPipeline);

        auto descriptorSets = m_gltfDescriptorSets.getAllSets(m_currentFrame);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_gltfPipelineLayout.get(), 0,
                                static_cast<uint32_t>(descriptorSets.size()),
                                descriptorSets.data(), 0, nullptr);

        m_gltfModel.draw(cmd, m_gltfPipelineLayout.get(), m_currentFrame);
    }

    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_syncObjects.getImageAvailableSemaphore(m_currentFrame) };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

    VkSemaphore signalSemaphores[] = { m_syncObjects.getRenderFinishedSemaphore(m_currentFrame) };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    if (vkQueueSubmit(m_device.graphicsQ(), 1, &submitInfo, m_syncObjects.getInFlightFence(m_currentFrame)) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = { m_swapchain.get()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(m_device.presentQ(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
        m_framebufferResized = false;
        m_swapchain.recreate(m_device, m_surface.get(), m_window);
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    m_currentFrame = (m_currentFrame + 1) % kMaxFramesInFlight;
}

// ============================================================================
// Cleanup
// ============================================================================

void Application::cleanup() {
    m_swapchain.destroy(m_device);
    m_baseTexture.destroy(m_device);
    m_normalTexture.destroy(m_device);
    m_metalRoughnessTexture.destroy(m_device);
    m_aoTexture.destroy(m_device);
    m_emissiveTexture.destroy(m_device);
    m_cubemap.destroy(m_device);
    m_vertexBuf.destroy(m_device);
    m_indexBuf.destroy(m_device);
    m_uboSet.destroy(m_device);

    m_descriptorPool.destroy(m_device);
    m_descriptorSetLayout.destroy(m_device);

    m_graphicsPipeline.destroy(m_device);
    m_pipelineLayout.destroy(m_device);

    // glTF resources
    m_gltfModel.destroy(m_device);
    m_gltfDescriptorSets.destroy();
    m_gltfDescriptorPool.destroy(m_device);
    m_gltfDescriptorLayouts.destroy(m_device);
    if (m_gltfPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device.get(), m_gltfPipeline, nullptr);
        m_gltfPipeline = VK_NULL_HANDLE;
    }
    m_gltfPipelineLayout.destroy(m_device);

    m_renderPass.destroy(m_device);

    m_framebuffer.destroy(m_device);

    m_syncObjects.destroy(m_device);

    m_commandPool.destroy(m_device);
    m_surface.destroy(m_instance.get());

    m_device.destroy();
    m_instance.destroy();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

