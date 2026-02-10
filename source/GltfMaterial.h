#pragma once
#include <glm/glm.hpp>
#include <string>

// ============================================================================
// glTF PBR Material System
// Supports glTF 2.0 PBR Metallic-Roughness workflow
// ============================================================================

// GPU-side material data (must match shader struct layout)
// Aligned to std140/std430 rules
struct MaterialData {
    glm::vec4 baseColorFactor;       // RGBA (default: 1,1,1,1)
    glm::vec4 emissiveFactor;        // RGB + padding (default: 0,0,0,0)

    float metallicFactor;            // 0.0 = dielectric, 1.0 = metal (default: 1.0)
    float roughnessFactor;           // 0.0 = smooth, 1.0 = rough (default: 1.0)
    float normalScale;               // Normal map intensity (default: 1.0)
    float occlusionStrength;         // AO strength (default: 1.0)

    float alphaCutoff;               // For alpha masking (default: 0.5)
    int alphaMode;                   // 0=OPAQUE, 1=MASK, 2=BLEND (default: 0)
    int doubleSided;                 // 0=false, 1=true (default: 0)
    int padding;                     // Alignment padding

    // Texture indices into texture array (-1 = no texture)
    int baseColorTextureIndex;       // Base color (albedo) texture index
    int metallicRoughnessTextureIndex; // Metallic-roughness texture index
    int normalTextureIndex;          // Normal map texture index
    int occlusionTextureIndex;       // Occlusion texture index

    int emissiveTextureIndex;        // Emissive texture index
    int baseColorTexCoord;           // UV set index for base color (0 or 1)
    int metallicRoughnessTexCoord;   // UV set index for metallic-roughness
    int normalTexCoord;              // UV set index for normal

    int occlusionTexCoord;           // UV set index for occlusion
    int emissiveTexCoord;            // UV set index for emissive
    int padding2;                    // Padding
    int padding3;                    // Padding
};

// Alpha blend modes
enum class AlphaMode {
    Opaque = 0,
    Mask = 1,
    Blend = 2
};

// CPU-side material with texture references
class GltfMaterial {
public:
    // Material properties (GPU data)
    MaterialData data;

    // Texture indices into GltfModel's texture array (-1 = no texture)
    int baseColorTextureIndex = -1;           // Base color (albedo) texture
    int metallicRoughnessTextureIndex = -1;   // Metallic (B) + Roughness (G) texture
    int normalTextureIndex = -1;              // Normal map (tangent space)
    int occlusionTextureIndex = -1;           // Ambient occlusion (R channel)
    int emissiveTextureIndex = -1;            // Emissive texture

    // Texture coordinate set indices (TEXCOORD_0 or TEXCOORD_1)
    int baseColorTexCoord = 0;
    int metallicRoughnessTexCoord = 0;
    int normalTexCoord = 0;
    int occlusionTexCoord = 0;
    int emissiveTexCoord = 0;

    // Material name (for debugging)
    std::string name;

    // Constructor with PBR defaults
    GltfMaterial();

    // Helper methods
    bool hasBaseColorTexture() const { return baseColorTextureIndex >= 0; }
    bool hasMetallicRoughnessTexture() const { return metallicRoughnessTextureIndex >= 0; }
    bool hasNormalTexture() const { return normalTextureIndex >= 0; }
    bool hasOcclusionTexture() const { return occlusionTextureIndex >= 0; }
    bool hasEmissiveTexture() const { return emissiveTextureIndex >= 0; }

    AlphaMode getAlphaMode() const { return static_cast<AlphaMode>(data.alphaMode); }
    bool isOpaque() const { return data.alphaMode == 0; }
    bool isDoubleSided() const { return data.doubleSided != 0; }
};
