#include "GltfMesh.h"

void GltfMesh::destroy(const Device& device) {
    for (auto& primitive : primitives) {
        primitive.destroy(device);
    }
    primitives.clear();
    weights.clear();
}

uint32_t GltfMesh::getTotalVertexCount() const {
    uint32_t total = 0;
    for (const auto& primitive : primitives) {
        total += primitive.vertexCount;
    }
    return total;
}

uint32_t GltfMesh::getTotalIndexCount() const {
    uint32_t total = 0;
    for (const auto& primitive : primitives) {
        total += primitive.indexCount;
    }
    return total;
}
