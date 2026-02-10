#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

// ============================================================================
// Extended Vertex Structure for glTF Models
// Supports all standard glTF vertex attributes including skinning and morphing
// ============================================================================

struct GltfVertex {
    glm::vec3 pos;           // POSITION (required)
    glm::vec3 normal;        // NORMAL
    glm::vec2 texCoord0;     // TEXCOORD_0 (primary UV)
    glm::vec2 texCoord1;     // TEXCOORD_1 (secondary UV for lightmaps, etc.)
    glm::vec4 color;         // COLOR_0 (vertex color with alpha)
    glm::vec4 tangent;       // TANGENT (xyz = tangent, w = handedness)
    glm::vec4 joints;        // JOINTS_0 (joint indices for skinning, 4 influences)
    glm::vec4 weights;       // WEIGHTS_0 (joint weights for skinning, 4 influences)

    // Vertex input binding description
    static VkVertexInputBindingDescription getBindingDescription();

    // Vertex attribute descriptions
    static std::array<VkVertexInputAttributeDescription, 8> getAttributeDescriptions();

    // Equality operator for deduplication
    bool operator==(const GltfVertex& other) const {
        return pos == other.pos &&
               normal == other.normal &&
               texCoord0 == other.texCoord0 &&
               texCoord1 == other.texCoord1 &&
               color == other.color &&
               tangent == other.tangent &&
               joints == other.joints &&
               weights == other.weights;
    }
};

// Hash function for use with unordered_map
namespace std {
    template<> struct hash<GltfVertex> {
        size_t operator()(GltfVertex const& vertex) const {
            size_t h1 = hash<float>()(vertex.pos.x);
            size_t h2 = hash<float>()(vertex.pos.y);
            size_t h3 = hash<float>()(vertex.pos.z);
            size_t h4 = hash<float>()(vertex.normal.x);
            size_t h5 = hash<float>()(vertex.texCoord0.x);
            size_t h6 = hash<float>()(vertex.texCoord0.y);
            return ((h1 ^ (h2 << 1)) ^ (h3 << 2)) ^ ((h4 << 3) ^ ((h5 << 4) ^ (h6 << 5)));
        }
    };
}
