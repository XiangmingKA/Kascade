// ============================================================================
// GltfModel.cpp - glTF Model Loader Implementation
// Main file for parsing and loading glTF 2.0 models
// ============================================================================

#include "GltfModel.h"
#include "Utilities.h"

// Include tinygltf library
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <cstring>

// ============================================================================
// Main Loading Function
// ============================================================================

void GltfModel::loadFromFile(const Device& device,
                               VkCommandPool cmdPool,
                               VkQueue queue,
                               const std::string& filename) {
    m_modelPath = filename;

    // Parse glTF file using tinygltf
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    // Determine if binary (.glb) or ASCII (.gltf)
    bool isBinary = (filename.substr(filename.find_last_of(".") + 1) == "glb");

    bool success = isBinary
        ? loader.LoadBinaryFromFile(&model, &err, &warn, filename)
        : loader.LoadASCIIFromFile(&model, &err, &warn, filename);

    if (!warn.empty()) {
        std::cerr << "glTF Warning: " << warn << std::endl;
    }

    if (!err.empty()) {
        throw std::runtime_error("glTF Error: " + err);
    }

    if (!success) {
        throw std::runtime_error("Failed to load glTF file: " + filename);
    }

    // Extract base directory for texture loading
    std::string baseDir = filename.substr(0, filename.find_last_of("/\\") + 1);

    // Load all components in order
    std::cout << "Loading glTF model: " << filename << std::endl;
    std::cout << "  Nodes: " << model.nodes.size() << std::endl;
    std::cout << "  Meshes: " << model.meshes.size() << std::endl;
    std::cout << "  Materials: " << model.materials.size() << std::endl;
    std::cout << "  Textures: " << model.textures.size() << std::endl;

    // First, determine texture usage from materials
    std::vector<bool> isColorTexture(model.textures.size(), false);
    std::cout << "Determining texture formats..." << std::endl;
    for (const auto& gltfMat : model.materials) {
        // Base color and emissive textures should use SRGB
        if (gltfMat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
            int idx = gltfMat.pbrMetallicRoughness.baseColorTexture.index;
            isColorTexture[idx] = true;
            std::cout << "  Texture " << idx << " is base color (SRGB)" << std::endl;
        }
        if (gltfMat.emissiveTexture.index >= 0) {
            int idx = gltfMat.emissiveTexture.index;
            isColorTexture[idx] = true;
            std::cout << "  Texture " << idx << " is emissive (SRGB)" << std::endl;
        }
        // Normal, metallic-roughness, and occlusion textures are NOT color textures (use LINEAR)
    }

    loadTextures(model, device, cmdPool, queue, baseDir, isColorTexture);
    loadSamplers(model, device);
    loadMaterials(model);
    loadMeshes(model, device, cmdPool, queue);
    loadNodes(model);

    // Create GPU buffers
    createMaterialBuffer(device);
    createTransformBuffer(device);

    // Initialize transforms
    updateTransforms(device);

    std::cout << "glTF model loaded successfully!" << std::endl;
}

// ============================================================================
// Cleanup
// ============================================================================

void GltfModel::destroy(const Device& device) {
    // Destroy meshes (which destroys primitives)
    for (auto& mesh : m_meshes) {
        mesh.destroy(device);
    }
    m_meshes.clear();

    // Destroy textures
    for (auto& texture : m_textures) {
        texture.destroy(device);
    }
    m_textures.clear();

    // Destroy samplers
    for (auto sampler : m_samplers) {
        if (sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device.get(), sampler, nullptr);
        }
    }
    m_samplers.clear();

    // Destroy GPU buffers
    m_materialBuffer.destroy(device);
    m_transformBuffer.destroy(device);

    // Clear data
    m_nodes.clear();
    m_rootNodes.clear();
    m_materials.clear();
}

// ============================================================================
// Texture Loading
// ============================================================================

void GltfModel::loadTextures(const tinygltf::Model& model,
                               const Device& device,
                               VkCommandPool cmdPool,
                               VkQueue queue,
                               const std::string& baseDir,
                               const std::vector<bool>& isColorTexture) {
    m_textures.resize(model.textures.size());

    std::cout << "Loading " << model.textures.size() << " textures:" << std::endl;

    for (size_t i = 0; i < model.textures.size(); ++i) {
        const tinygltf::Texture& gltfTex = model.textures[i];
        if (gltfTex.source < 0 || gltfTex.source >= static_cast<int>(model.images.size())) {
            std::cerr << "Warning: Texture " << i << " has invalid image reference" << std::endl;
            continue;
        }

        const tinygltf::Image& gltfImage = model.images[gltfTex.source];
        std::cout << "  Texture " << i << ": " << (gltfImage.uri.empty() ? "embedded" : gltfImage.uri);

        // Use SRGB for color textures (base color, emissive), LINEAR for data textures (normal, metallic-roughness, occlusion)
        VkFormat format = isColorTexture[i] ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
        std::cout << " [" << (isColorTexture[i] ? "SRGB" : "LINEAR") << "]" << std::endl;

        // Load from embedded data or external file
        if (!gltfImage.image.empty()) {
            // Image data is embedded
            int width = gltfImage.width;
            int height = gltfImage.height;
            const unsigned char* pixels = gltfImage.image.data();

            m_textures[i].createFromPixels(device, cmdPool, queue,
                                            pixels, width, height,
                                            format, true,
                                            VK_FILTER_LINEAR,
                                            VK_SAMPLER_ADDRESS_MODE_REPEAT);
        } else if (!gltfImage.uri.empty()) {
            // Image is external file - load using stb_image
            std::string imagePath = baseDir + gltfImage.uri;

            int width, height, channels;
            stbi_uc* pixels = stbi_load(imagePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

            if (!pixels) {
                std::cerr << "Warning: Failed to load external image: " << imagePath << std::endl;
                continue;
            }

            m_textures[i].createFromPixels(device, cmdPool, queue,
                                            pixels, width, height,
                                            format, true,
                                            VK_FILTER_LINEAR,
                                            VK_SAMPLER_ADDRESS_MODE_REPEAT);

            stbi_image_free(pixels);
        }
    }
}

// ============================================================================
// Sampler Loading
// ============================================================================

void GltfModel::loadSamplers(const tinygltf::Model& model, const Device& device) {
    m_samplers.resize(model.samplers.size(), VK_NULL_HANDLE);

    for (size_t i = 0; i < model.samplers.size(); ++i) {
        const tinygltf::Sampler& gltfSampler = model.samplers[i];

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        // Convert glTF filter modes to Vulkan
        auto getFilter = [](int gltfFilter) -> VkFilter {
            switch (gltfFilter) {
                case 9728: return VK_FILTER_NEAREST; // NEAREST
                case 9729: return VK_FILTER_LINEAR;  // LINEAR
                case 9984: return VK_FILTER_NEAREST; // NEAREST_MIPMAP_NEAREST
                case 9985: return VK_FILTER_LINEAR;  // LINEAR_MIPMAP_NEAREST
                case 9986: return VK_FILTER_NEAREST; // NEAREST_MIPMAP_LINEAR
                case 9987: return VK_FILTER_LINEAR;  // LINEAR_MIPMAP_LINEAR
                default: return VK_FILTER_LINEAR;
            }
        };

        auto getWrapMode = [](int gltfWrap) -> VkSamplerAddressMode {
            switch (gltfWrap) {
                case 10497: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
                case 33071: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                case 33648: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            }
        };

        samplerInfo.magFilter = getFilter(gltfSampler.magFilter);
        samplerInfo.minFilter = getFilter(gltfSampler.minFilter);
        samplerInfo.addressModeU = getWrapMode(gltfSampler.wrapS);
        samplerInfo.addressModeV = getWrapMode(gltfSampler.wrapT);
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        samplerInfo.mipLodBias = 0.0f;

        if (vkCreateSampler(device.get(), &samplerInfo, nullptr, &m_samplers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create glTF sampler");
        }
    }
}

// ============================================================================
// Material Loading
// ============================================================================

void GltfModel::loadMaterials(const tinygltf::Model& model) {
    m_materials.resize(model.materials.size());

    for (size_t i = 0; i < model.materials.size(); ++i) {
        const tinygltf::Material& gltfMat = model.materials[i];
        GltfMaterial& material = m_materials[i];

        material.name = gltfMat.name;

        // PBR Metallic-Roughness
        const auto& pbr = gltfMat.pbrMetallicRoughness;

        // Base color
        material.data.baseColorFactor = glm::vec4(
            static_cast<float>(pbr.baseColorFactor[0]),
            static_cast<float>(pbr.baseColorFactor[1]),
            static_cast<float>(pbr.baseColorFactor[2]),
            static_cast<float>(pbr.baseColorFactor[3])
        );

        material.data.metallicFactor = static_cast<float>(pbr.metallicFactor);
        material.data.roughnessFactor = static_cast<float>(pbr.roughnessFactor);

        // Emissive
        material.data.emissiveFactor = glm::vec4(
            static_cast<float>(gltfMat.emissiveFactor[0]),
            static_cast<float>(gltfMat.emissiveFactor[1]),
            static_cast<float>(gltfMat.emissiveFactor[2]),
            0.0f
        );

        // Alpha mode
        if (gltfMat.alphaMode == "OPAQUE") {
            material.data.alphaMode = 0;
        } else if (gltfMat.alphaMode == "MASK") {
            material.data.alphaMode = 1;
        } else if (gltfMat.alphaMode == "BLEND") {
            material.data.alphaMode = 2;
        }

        material.data.alphaCutoff = static_cast<float>(gltfMat.alphaCutoff);
        material.data.doubleSided = gltfMat.doubleSided ? 1 : 0;

        // Textures - store indices in both CPU and GPU data
        if (pbr.baseColorTexture.index >= 0) {
            material.baseColorTextureIndex = pbr.baseColorTexture.index;
            material.baseColorTexCoord = pbr.baseColorTexture.texCoord;
            material.data.baseColorTextureIndex = pbr.baseColorTexture.index;
            material.data.baseColorTexCoord = pbr.baseColorTexture.texCoord;
        }

        if (pbr.metallicRoughnessTexture.index >= 0) {
            material.metallicRoughnessTextureIndex = pbr.metallicRoughnessTexture.index;
            material.metallicRoughnessTexCoord = pbr.metallicRoughnessTexture.texCoord;
            material.data.metallicRoughnessTextureIndex = pbr.metallicRoughnessTexture.index;
            material.data.metallicRoughnessTexCoord = pbr.metallicRoughnessTexture.texCoord;
        }

        if (gltfMat.normalTexture.index >= 0) {
            material.normalTextureIndex = gltfMat.normalTexture.index;
            material.normalTexCoord = gltfMat.normalTexture.texCoord;
            material.data.normalTextureIndex = gltfMat.normalTexture.index;
            material.data.normalTexCoord = gltfMat.normalTexture.texCoord;
            material.data.normalScale = static_cast<float>(gltfMat.normalTexture.scale);
        }

        if (gltfMat.occlusionTexture.index >= 0) {
            material.occlusionTextureIndex = gltfMat.occlusionTexture.index;
            material.occlusionTexCoord = gltfMat.occlusionTexture.texCoord;
            material.data.occlusionTextureIndex = gltfMat.occlusionTexture.index;
            material.data.occlusionTexCoord = gltfMat.occlusionTexture.texCoord;
            material.data.occlusionStrength = static_cast<float>(gltfMat.occlusionTexture.strength);
        }

        if (gltfMat.emissiveTexture.index >= 0) {
            material.emissiveTextureIndex = gltfMat.emissiveTexture.index;
            material.emissiveTexCoord = gltfMat.emissiveTexture.texCoord;
            material.data.emissiveTextureIndex = gltfMat.emissiveTexture.index;
            material.data.emissiveTexCoord = gltfMat.emissiveTexture.texCoord;
        }
    }

    // Add default material if none exist
    if (m_materials.empty()) {
        m_materials.push_back(GltfMaterial());
    }
}

// ============================================================================
// Mesh Loading (Vertex Extraction is CRITICAL)
// ============================================================================

void GltfModel::loadMeshes(const tinygltf::Model& model,
                             const Device& device,
                             VkCommandPool cmdPool,
                             VkQueue queue) {
    m_meshes.resize(model.meshes.size());

    for (size_t i = 0; i < model.meshes.size(); ++i) {
        const tinygltf::Mesh& gltfMesh = model.meshes[i];
        GltfMesh& mesh = m_meshes[i];

        mesh.name = gltfMesh.name;
        mesh.primitives.resize(gltfMesh.primitives.size());

        for (size_t p = 0; p < gltfMesh.primitives.size(); ++p) {
            const tinygltf::Primitive& gltfPrim = gltfMesh.primitives[p];
            GltfPrimitive& primitive = mesh.primitives[p];

            // Extract vertex and index data
            std::vector<GltfVertex> vertices;
            std::vector<uint32_t> indices;

            extractVertexData(model, gltfPrim, vertices, indices);

            // Create GPU buffers for this primitive
            primitive.create(device, cmdPool, queue, vertices, indices);
            primitive.materialIndex = gltfPrim.material;
        }
    }
}

// ============================================================================
// CRITICAL: Vertex Data Extraction from glTF
// ============================================================================

void GltfModel::extractVertexData(const tinygltf::Model& model,
                                    const tinygltf::Primitive& primitive,
                                    std::vector<GltfVertex>& vertices,
                                    std::vector<uint32_t>& indices) {
    // Extract POSITION (required)
    auto posIt = primitive.attributes.find("POSITION");
    if (posIt == primitive.attributes.end()) {
        throw std::runtime_error("glTF primitive missing required POSITION attribute");
    }

    const tinygltf::Accessor& posAccessor = model.accessors[posIt->second];
    size_t vertexCount = posAccessor.count;
    vertices.resize(vertexCount);

    // Extract positions
    {
        const tinygltf::BufferView& bufferView = model.bufferViews[posAccessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
        const float* positions = reinterpret_cast<const float*>(
            &buffer.data[bufferView.byteOffset + posAccessor.byteOffset]);

        for (size_t i = 0; i < vertexCount; ++i) {
            vertices[i].pos = glm::vec3(positions[i * 3 + 0],
                                         positions[i * 3 + 1],
                                         positions[i * 3 + 2]);
        }
    }

    // Extract NORMAL (optional, default to (0,0,1))
    auto normIt = primitive.attributes.find("NORMAL");
    if (normIt != primitive.attributes.end()) {
        const tinygltf::Accessor& normAccessor = model.accessors[normIt->second];
        const tinygltf::BufferView& bufferView = model.bufferViews[normAccessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
        const float* normals = reinterpret_cast<const float*>(
            &buffer.data[bufferView.byteOffset + normAccessor.byteOffset]);

        for (size_t i = 0; i < vertexCount; ++i) {
            vertices[i].normal = glm::vec3(normals[i * 3 + 0],
                                            normals[i * 3 + 1],
                                            normals[i * 3 + 2]);
        }
    } else {
        for (size_t i = 0; i < vertexCount; ++i) {
            vertices[i].normal = glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }

    // Extract TEXCOORD_0 (optional)
    auto uv0It = primitive.attributes.find("TEXCOORD_0");
    if (uv0It != primitive.attributes.end()) {
        const tinygltf::Accessor& uvAccessor = model.accessors[uv0It->second];
        const tinygltf::BufferView& bufferView = model.bufferViews[uvAccessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
        const float* uvs = reinterpret_cast<const float*>(
            &buffer.data[bufferView.byteOffset + uvAccessor.byteOffset]);

        for (size_t i = 0; i < vertexCount; ++i) {
            vertices[i].texCoord0 = glm::vec2(uvs[i * 2 + 0], uvs[i * 2 + 1]);
        }
    } else {
        for (size_t i = 0; i < vertexCount; ++i) {
            vertices[i].texCoord0 = glm::vec2(0.0f);
        }
    }

    // Extract TEXCOORD_1 (optional - secondary UV set)
    auto uv1It = primitive.attributes.find("TEXCOORD_1");
    if (uv1It != primitive.attributes.end()) {
        const tinygltf::Accessor& uvAccessor = model.accessors[uv1It->second];
        const tinygltf::BufferView& bufferView = model.bufferViews[uvAccessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
        const float* uvs = reinterpret_cast<const float*>(
            &buffer.data[bufferView.byteOffset + uvAccessor.byteOffset]);

        for (size_t i = 0; i < vertexCount; ++i) {
            vertices[i].texCoord1 = glm::vec2(uvs[i * 2 + 0], uvs[i * 2 + 1]);
        }
    } else {
        for (size_t i = 0; i < vertexCount; ++i) {
            vertices[i].texCoord1 = glm::vec2(0.0f);
        }
    }

    // Extract COLOR_0 (optional - vertex colors)
    auto colorIt = primitive.attributes.find("COLOR_0");
    if (colorIt != primitive.attributes.end()) {
        const tinygltf::Accessor& colorAccessor = model.accessors[colorIt->second];
        const tinygltf::BufferView& bufferView = model.bufferViews[colorAccessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

        // Colors can be VEC3 or VEC4, and can be float or normalized byte
        if (colorAccessor.type == TINYGLTF_TYPE_VEC4) {
            if (colorAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                const float* colors = reinterpret_cast<const float*>(
                    &buffer.data[bufferView.byteOffset + colorAccessor.byteOffset]);
                for (size_t i = 0; i < vertexCount; ++i) {
                    vertices[i].color = glm::vec4(colors[i * 4 + 0], colors[i * 4 + 1],
                                                   colors[i * 4 + 2], colors[i * 4 + 3]);
                }
            }
        } else if (colorAccessor.type == TINYGLTF_TYPE_VEC3) {
            if (colorAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                const float* colors = reinterpret_cast<const float*>(
                    &buffer.data[bufferView.byteOffset + colorAccessor.byteOffset]);
                for (size_t i = 0; i < vertexCount; ++i) {
                    vertices[i].color = glm::vec4(colors[i * 3 + 0], colors[i * 3 + 1],
                                                   colors[i * 3 + 2], 1.0f);
                }
            }
        }
    } else {
        for (size_t i = 0; i < vertexCount; ++i) {
            vertices[i].color = glm::vec4(1.0f);
        }
    }

    // Extract TANGENT (optional - for normal mapping)
    auto tangentIt = primitive.attributes.find("TANGENT");
    if (tangentIt != primitive.attributes.end()) {
        const tinygltf::Accessor& tangentAccessor = model.accessors[tangentIt->second];
        const tinygltf::BufferView& bufferView = model.bufferViews[tangentAccessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
        const float* tangents = reinterpret_cast<const float*>(
            &buffer.data[bufferView.byteOffset + tangentAccessor.byteOffset]);

        for (size_t i = 0; i < vertexCount; ++i) {
            vertices[i].tangent = glm::vec4(tangents[i * 4 + 0], tangents[i * 4 + 1],
                                             tangents[i * 4 + 2], tangents[i * 4 + 3]);
        }
    } else {
        // Default tangent if not provided (will be generated later if needed)
        for (size_t i = 0; i < vertexCount; ++i) {
            vertices[i].tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    // Extract JOINTS_0 and WEIGHTS_0 (for skinning - optional)
    auto jointsIt = primitive.attributes.find("JOINTS_0");
    auto weightsIt = primitive.attributes.find("WEIGHTS_0");

    if (jointsIt != primitive.attributes.end() && weightsIt != primitive.attributes.end()) {
        // Extract joints
        const tinygltf::Accessor& jointsAccessor = model.accessors[jointsIt->second];
        const tinygltf::BufferView& jointsBufferView = model.bufferViews[jointsAccessor.bufferView];
        const tinygltf::Buffer& jointsBuffer = model.buffers[jointsBufferView.buffer];

        // Extract weights
        const tinygltf::Accessor& weightsAccessor = model.accessors[weightsIt->second];
        const tinygltf::BufferView& weightsBufferView = model.bufferViews[weightsAccessor.bufferView];
        const tinygltf::Buffer& weightsBuffer = model.buffers[weightsBufferView.buffer];

        // Joints can be unsigned byte or unsigned short
        if (jointsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            const uint16_t* joints = reinterpret_cast<const uint16_t*>(
                &jointsBuffer.data[jointsBufferView.byteOffset + jointsAccessor.byteOffset]);

            for (size_t i = 0; i < vertexCount; ++i) {
                vertices[i].joints = glm::vec4(joints[i * 4 + 0], joints[i * 4 + 1],
                                                joints[i * 4 + 2], joints[i * 4 + 3]);
            }
        } else if (jointsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            const uint8_t* joints = reinterpret_cast<const uint8_t*>(
                &jointsBuffer.data[jointsBufferView.byteOffset + jointsAccessor.byteOffset]);

            for (size_t i = 0; i < vertexCount; ++i) {
                vertices[i].joints = glm::vec4(joints[i * 4 + 0], joints[i * 4 + 1],
                                                joints[i * 4 + 2], joints[i * 4 + 3]);
            }
        }

        // Weights are always float
        const float* weights = reinterpret_cast<const float*>(
            &weightsBuffer.data[weightsBufferView.byteOffset + weightsAccessor.byteOffset]);

        for (size_t i = 0; i < vertexCount; ++i) {
            vertices[i].weights = glm::vec4(weights[i * 4 + 0], weights[i * 4 + 1],
                                             weights[i * 4 + 2], weights[i * 4 + 3]);
        }
    } else {
        // No skinning data
        for (size_t i = 0; i < vertexCount; ++i) {
            vertices[i].joints = glm::vec4(0.0f);
            vertices[i].weights = glm::vec4(0.0f);
        }
    }

    // Extract indices
    if (primitive.indices >= 0) {
        const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
        const tinygltf::BufferView& bufferView = model.bufferViews[indexAccessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

        indices.resize(indexAccessor.count);

        const void* dataPtr = &buffer.data[bufferView.byteOffset + indexAccessor.byteOffset];

        switch (indexAccessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
                for (size_t i = 0; i < indexAccessor.count; ++i) {
                    indices[i] = buf[i];
                }
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
                for (size_t i = 0; i < indexAccessor.count; ++i) {
                    indices[i] = buf[i];
                }
                break;
            }
            default:
                throw std::runtime_error("Unsupported index component type");
        }
    }

    // Note: Tangent generation is handled above in the TANGENT extraction section
    // If tangents are missing, they default to (1,0,0,1) and can be generated if needed
}

// ============================================================================
// Tangent Generation
// ============================================================================

void GltfModel::generateTangents(std::vector<GltfVertex>& vertices,
                                   const std::vector<uint32_t>& indices) {
    // TODO: Implement tangent generation (similar to Application::computeTangents)
    // For now, use default tangents
    for (auto& vertex : vertices) {
        vertex.tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }
}

// ============================================================================
// Node Loading (Scene Graph)
// ============================================================================

void GltfModel::loadNodes(const tinygltf::Model& model) {
    m_nodes.resize(model.nodes.size());

    // First pass: create nodes and extract properties
    for (size_t i = 0; i < model.nodes.size(); ++i) {
        const tinygltf::Node& gltfNode = model.nodes[i];
        GltfNode& node = m_nodes[i];

        node.index = static_cast<int>(i);
        node.name = gltfNode.name;
        node.meshIndex = gltfNode.mesh;
        node.skinIndex = gltfNode.skin;
        node.cameraIndex = gltfNode.camera;

        // Extract transform
        if (!gltfNode.matrix.empty()) {
            // Matrix form
            node.useTRS = false;
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    node.matrix[col][row] = static_cast<float>(gltfNode.matrix[row * 4 + col]);
                }
            }
        } else {
            // TRS form
            node.useTRS = true;

            if (!gltfNode.translation.empty()) {
                node.translation = glm::vec3(
                    static_cast<float>(gltfNode.translation[0]),
                    static_cast<float>(gltfNode.translation[1]),
                    static_cast<float>(gltfNode.translation[2])
                );
            }

            if (!gltfNode.rotation.empty()) {
                node.rotation = glm::quat(
                    static_cast<float>(gltfNode.rotation[3]),  // w
                    static_cast<float>(gltfNode.rotation[0]),  // x
                    static_cast<float>(gltfNode.rotation[1]),  // y
                    static_cast<float>(gltfNode.rotation[2])   // z
                );
            }

            if (!gltfNode.scale.empty()) {
                node.scale = glm::vec3(
                    static_cast<float>(gltfNode.scale[0]),
                    static_cast<float>(gltfNode.scale[1]),
                    static_cast<float>(gltfNode.scale[2])
                );
            }
        }

        // Store children
        node.children.resize(gltfNode.children.size());
        for (size_t c = 0; c < gltfNode.children.size(); ++c) {
            node.children[c] = gltfNode.children[c];
        }
    }

    // Second pass: establish parent-child relationships
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        for (int childIndex : m_nodes[i].children) {
            if (childIndex >= 0 && childIndex < static_cast<int>(m_nodes.size())) {
                m_nodes[childIndex].parent = static_cast<int>(i);
            }
        }
    }

    // Identify root nodes
    const tinygltf::Scene& scene = model.scenes[model.defaultScene >= 0 ? model.defaultScene : 0];
    m_rootNodes.resize(scene.nodes.size());
    for (size_t i = 0; i < scene.nodes.size(); ++i) {
        m_rootNodes[i] = scene.nodes[i];
    }
}

// ============================================================================
// GPU Buffer Creation
// ============================================================================

void GltfModel::createMaterialBuffer(const Device& device) {
    if (m_materials.empty()) return;

    VkDeviceSize bufferSize = sizeof(MaterialData) * m_materials.size();

    // Create host-visible storage buffer (simpler for dynamic updates)
    // Storage buffers can be host-visible and still efficiently accessed by shaders
    m_materialBuffer.create(device, bufferSize,
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Copy material data directly to storage buffer
    void* dest = m_materialBuffer.map(device);
    for (size_t i = 0; i < m_materials.size(); ++i) {
        std::cout << "Uploading material " << i << " (" << m_materials[i].name << "):" << std::endl;
        std::cout << "  baseColorTexIdx: " << m_materials[i].data.baseColorTextureIndex << std::endl;
        std::cout << "  metalRoughTexIdx: " << m_materials[i].data.metallicRoughnessTextureIndex << std::endl;
        std::cout << "  normalTexIdx: " << m_materials[i].data.normalTextureIndex << std::endl;

        memcpy(static_cast<char*>(dest) + i * sizeof(MaterialData),
               &m_materials[i].data,
               sizeof(MaterialData));
    }
    m_materialBuffer.unmap(device);
}

void GltfModel::createTransformBuffer(const Device& device) {
    if (m_nodes.empty()) return;

    VkDeviceSize bufferSize = sizeof(glm::mat4) * m_nodes.size();

    // Create host-visible storage buffer for dynamic transform updates
    m_transformBuffer.create(device, bufferSize,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Initialize with identity matrices
    void* data = m_transformBuffer.map(device);
    std::vector<glm::mat4> matrices(m_nodes.size(), glm::mat4(1.0f));
    memcpy(data, matrices.data(), bufferSize);
    m_transformBuffer.unmap(device);
}

// ============================================================================
// Transform Update
// ============================================================================

void GltfModel::updateTransforms(const Device& device) {
    if (m_nodes.empty() || !m_transformBuffer.get()) return;

    // Mark all nodes as needing transform update
    for (auto& node : m_nodes) {
        node.markDirty();
    }

    // Compute world transforms for all nodes
    std::vector<glm::mat4> worldMatrices(m_nodes.size());
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        worldMatrices[i] = m_nodes[i].getWorldMatrix(m_nodes);
    }

    // Upload to GPU buffer
    void* data = m_transformBuffer.map(device);
    memcpy(data, worldMatrices.data(), sizeof(glm::mat4) * worldMatrices.size());
    m_transformBuffer.unmap(device);
}

// ============================================================================
// Rendering
// ============================================================================

void GltfModel::draw(VkCommandBuffer cmd,
                      VkPipelineLayout pipelineLayout,
                      uint32_t currentFrame) const {
    // Draw all root nodes (which recursively draws children)
    for (int rootIndex : m_rootNodes) {
        if (rootIndex >= 0 && rootIndex < static_cast<int>(m_nodes.size())) {
            drawNode(cmd, pipelineLayout, rootIndex);
        }
    }
}

void GltfModel::drawNode(VkCommandBuffer cmd,
                          VkPipelineLayout pipelineLayout,
                          int nodeIndex) const {
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(m_nodes.size())) {
        return;
    }

    const GltfNode& node = m_nodes[nodeIndex];

    // Draw mesh if this node has one
    if (node.meshIndex >= 0 && node.meshIndex < static_cast<int>(m_meshes.size())) {
        const GltfMesh& mesh = m_meshes[node.meshIndex];

        for (const GltfPrimitive& primitive : mesh.primitives) {
            // Set push constants for node index and material index
            struct PushConstants {
                int nodeIndex;
                int materialIndex;
            } pushConstants;

            pushConstants.nodeIndex = nodeIndex;
            pushConstants.materialIndex = primitive.materialIndex;

            vkCmdPushConstants(cmd, pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(PushConstants), &pushConstants);

            primitive.draw(cmd);
        }
    }

    // Recursively draw children
    for (int childIndex : node.children) {
        drawNode(cmd, pipelineLayout, childIndex);
    }
}
