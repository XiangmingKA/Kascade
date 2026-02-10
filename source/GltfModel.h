#pragma once
#include "GltfNode.h"
#include "GltfMesh.h"
#include "GltfMaterial.h"
#include "GltfVertex.h"
#include "Texture.h"
#include "Buffer.h"
#include "Device.h"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

// Forward declare tinygltf types to avoid including in header
namespace tinygltf {
    class Model;
    struct Primitive;
    struct Accessor;
}

// ============================================================================
// glTF Model Loader and Manager
// Main class for loading and rendering glTF 2.0 models
// ============================================================================

class GltfModel {
public:
    GltfModel() = default;
    ~GltfModel() = default;

    // Load glTF model from file (.gltf or .glb)
    void loadFromFile(const Device& device,
                      VkCommandPool cmdPool,
                      VkQueue queue,
                      const std::string& filename);

    // Destroy all GPU resources
    void destroy(const Device& device);

    // Render the model
    void draw(VkCommandBuffer cmd,
              VkPipelineLayout pipelineLayout,
              uint32_t currentFrame) const;

    // Update all node world transforms (call before rendering if nodes changed)
    void updateTransforms(const Device& device);

    // Accessors
    const std::vector<GltfNode>& getNodes() const { return m_nodes; }
    const std::vector<GltfMesh>& getMeshes() const { return m_meshes; }
    const std::vector<GltfMaterial>& getMaterials() const { return m_materials; }
    const std::vector<Texture>& getTextures() const { return m_textures; }
    const std::vector<int>& getRootNodes() const { return m_rootNodes; }

    // Buffer accessors
    VkBuffer getMaterialBuffer() const { return m_materialBuffer.get(); }
    VkBuffer getTransformBuffer() const { return m_transformBuffer.get(); }

    // Model info
    std::string getModelPath() const { return m_modelPath; }
    bool isLoaded() const { return !m_nodes.empty(); }

private:
    // Scene data
    std::vector<GltfNode> m_nodes;
    std::vector<int> m_rootNodes;  // Indices of top-level nodes
    std::vector<GltfMesh> m_meshes;
    std::vector<GltfMaterial> m_materials;
    std::vector<Texture> m_textures;
    std::vector<VkSampler> m_samplers;

    // GPU buffers
    Buffer m_materialBuffer;   // Storage buffer: array of MaterialData
    Buffer m_transformBuffer;  // Storage buffer: array of mat4 (one per node)

    // Model info
    std::string m_modelPath;

    // Loading helper methods
    void loadTextures(const tinygltf::Model& model,
                      const Device& device,
                      VkCommandPool cmdPool,
                      VkQueue queue,
                      const std::string& baseDir,
                      const std::vector<bool>& isColorTexture);

    void loadSamplers(const tinygltf::Model& model, const Device& device);

    void loadMaterials(const tinygltf::Model& model);

    void loadMeshes(const tinygltf::Model& model,
                    const Device& device,
                    VkCommandPool cmdPool,
                    VkQueue queue);

    void loadNodes(const tinygltf::Model& model);

    void createMaterialBuffer(const Device& device);
    void createTransformBuffer(const Device& device);

    // Vertex extraction from glTF buffers
    void extractVertexData(const tinygltf::Model& model,
                           const tinygltf::Primitive& primitive,
                           std::vector<GltfVertex>& vertices,
                           std::vector<uint32_t>& indices);

    // Extract specific attribute from glTF accessor
    template<typename T>
    void extractAttribute(const tinygltf::Model& model,
                          const tinygltf::Accessor& accessor,
                          std::vector<T>& outData);

    // Generate tangents if not present in glTF
    void generateTangents(std::vector<GltfVertex>& vertices,
                          const std::vector<uint32_t>& indices);

    // Rendering helpers
    void drawNode(VkCommandBuffer cmd,
                  VkPipelineLayout pipelineLayout,
                  int nodeIndex) const;
};
