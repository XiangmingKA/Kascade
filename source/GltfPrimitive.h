#pragma once
#include "VertexIndexBuffers.h"
#include "Device.h"
#include "GltfVertex.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

// ============================================================================
// glTF Primitive
// Represents a single drawable geometry unit with a material
// A glTF mesh can contain multiple primitives with different materials
// ============================================================================

class GltfPrimitive {
public:
    // Geometry buffers
    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t firstIndex = 0;     // Offset into index buffer
    int32_t vertexOffset = 0;    // Offset into vertex buffer

    // Material reference
    int materialIndex = -1;  // Index into model's material array

    // Morph targets (for blend shapes - future use)
    std::vector<VertexBuffer> morphTargetBuffers;
    std::vector<float> morphWeights;

    // Create primitive from vertex/index data
    void create(const Device& device,
                VkCommandPool cmdPool,
                VkQueue queue,
                const std::vector<GltfVertex>& vertices,
                const std::vector<uint32_t>& indices);

    // Destroy GPU resources
    void destroy(const Device& device);

    // Draw this primitive
    void draw(VkCommandBuffer cmd) const;

    // Check if primitive has valid geometry
    bool isValid() const { return vertexCount > 0 && indexCount > 0; }
};
