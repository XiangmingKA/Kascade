#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

// ============================================================================
// glTF Scene Graph Node
// Represents a node in the glTF scene hierarchy with transform and references
// ============================================================================

class GltfNode {
public:
    // Node identification
    std::string name;
    int index = -1;  // Node index in the model's node array

    // Object references
    int meshIndex = -1;   // Index into model's mesh array (-1 = no mesh)
    int skinIndex = -1;   // Index into model's skin array (-1 = no skin)
    int cameraIndex = -1; // Index into model's camera array (-1 = no camera)

    // Transform representation (glTF supports TRS or matrix)
    glm::vec3 translation = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);  // Identity quaternion
    glm::vec3 scale = glm::vec3(1.0f);
    glm::mat4 matrix = glm::mat4(1.0f);  // Used if TRS is not provided
    bool useTRS = true;  // If false, use matrix directly

    // Scene graph hierarchy
    std::vector<int> children;  // Indices of child nodes
    int parent = -1;            // Index of parent node (-1 = root node)

    // Computed transform
    glm::mat4 getLocalMatrix() const;
    glm::mat4 getWorldMatrix(const std::vector<GltfNode>& allNodes) const;

    // Update cached world matrix (call when transforms change)
    void markDirty();

private:
    mutable glm::mat4 m_worldMatrixCache = glm::mat4(1.0f);
    mutable bool m_worldMatrixDirty = true;
};
