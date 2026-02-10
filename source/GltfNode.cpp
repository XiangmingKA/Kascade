#include "GltfNode.h"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 GltfNode::getLocalMatrix() const {
    if (useTRS) {
        // Build matrix from TRS components
        glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 R = glm::mat4_cast(rotation);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        return T * R * S;
    } else {
        // Use matrix directly
        return matrix;
    }
}

glm::mat4 GltfNode::getWorldMatrix(const std::vector<GltfNode>& allNodes) const {
    // Return cached value if still valid
    if (!m_worldMatrixDirty) {
        return m_worldMatrixCache;
    }

    // Compute local transform
    glm::mat4 localTransform = getLocalMatrix();

    // If this node has a parent, multiply by parent's world matrix
    if (parent >= 0 && parent < static_cast<int>(allNodes.size())) {
        m_worldMatrixCache = allNodes[parent].getWorldMatrix(allNodes) * localTransform;
    } else {
        // Root node - world matrix is just local matrix
        m_worldMatrixCache = localTransform;
    }

    m_worldMatrixDirty = false;
    return m_worldMatrixCache;
}

void GltfNode::markDirty() {
    m_worldMatrixDirty = true;
}
