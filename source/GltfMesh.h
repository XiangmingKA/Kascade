#pragma once
#include "GltfPrimitive.h"
#include "Device.h"
#include <string>
#include <vector>

// ============================================================================
// glTF Mesh
// Container for one or more primitives that make up a mesh
// Each primitive can have a different material
// ============================================================================

class GltfMesh {
public:
    std::string name;
    std::vector<GltfPrimitive> primitives;

    // Morph target weights (for blend shapes - future use)
    std::vector<float> weights;

    // Destroy all GPU resources
    void destroy(const Device& device);

    // Check if mesh has any valid primitives
    bool isValid() const { return !primitives.empty(); }

    // Get total vertex and index counts across all primitives
    uint32_t getTotalVertexCount() const;
    uint32_t getTotalIndexCount() const;
};
