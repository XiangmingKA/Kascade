#include "GltfMaterial.h"

GltfMaterial::GltfMaterial() {
    // Initialize with glTF PBR defaults
    data.baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // White, fully opaque
    data.emissiveFactor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);   // No emission

    data.metallicFactor = 1.0f;      // Fully metallic by default
    data.roughnessFactor = 1.0f;     // Fully rough by default
    data.normalScale = 1.0f;         // Full normal map strength
    data.occlusionStrength = 1.0f;   // Full AO strength

    data.alphaCutoff = 0.5f;         // Standard alpha test threshold
    data.alphaMode = 0;              // OPAQUE by default
    data.doubleSided = 0;            // Single-sided by default
    data.padding = 0;

    // Initialize texture indices to -1 (no texture)
    data.baseColorTextureIndex = -1;
    data.metallicRoughnessTextureIndex = -1;
    data.normalTextureIndex = -1;
    data.occlusionTextureIndex = -1;
    data.emissiveTextureIndex = -1;

    // Initialize UV set indices to 0 (primary UV set)
    data.baseColorTexCoord = 0;
    data.metallicRoughnessTexCoord = 0;
    data.normalTexCoord = 0;
    data.occlusionTexCoord = 0;
    data.emissiveTexCoord = 0;

    data.padding2 = 0;
    data.padding3 = 0;

    name = "default";
}
