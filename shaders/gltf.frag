#version 450

// ============================================================================
// glTF Fragment Shader
// PBR (Physically-Based Rendering) with glTF 2.0 metallic-roughness workflow
// ============================================================================

const float PI = 3.14159265359;

// Set 0: Per-frame data (camera, lights)
// NOTE: Must match UniformBufferObject in Application.cpp exactly
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;          // Not used in glTF
    mat4 view;
    mat4 proj;
    mat4 normalMat;      // Not used in glTF
    vec4 cameraPosition;
    vec4 viewDirection;
    vec4 sunDirection;
    vec4 sunColor;
    vec4 ambientLight;
} ubo;

// Set 1: Per-model material data
layout(set = 1, binding = 1) readonly buffer MaterialBuffer {
    // MaterialData structure (aligned to match C++ struct)
    // vec4 baseColorFactor
    // vec4 emissiveFactor
    // float metallicFactor, roughnessFactor, normalScale, occlusionStrength
    // float alphaCutoff, int alphaMode, int doubleSided, int padding
    // int baseColorTextureIndex, metallicRoughnessTextureIndex, normalTextureIndex, occlusionTextureIndex
    // int emissiveTextureIndex, baseColorTexCoord, metallicRoughnessTexCoord, normalTexCoord
    // int occlusionTexCoord, emissiveTexCoord, padding2, padding3
    vec4 materials[];  // Each material takes 7 vec4s (112 bytes)
};

// Set 2: Textures (array of combined image samplers)
// glTF materials can reference any texture by index
layout(set = 2, binding = 0) uniform sampler2D textures[32];  // Support up to 32 textures

// Set 3: Environment cubemap for IBL
layout(set = 3, binding = 0) uniform samplerCube cubemapTex;

// Push constants for material/node indices
layout(push_constant) uniform PushConstants {
    int nodeIndex;
    int materialIndex;
} pc;

// Inputs from vertex shader
layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragTangent;
layout(location = 3) in vec3 fragBitangent;
layout(location = 4) in vec2 fragTexCoord0;
layout(location = 5) in vec2 fragTexCoord1;
layout(location = 6) in vec4 fragColor;

// Output
layout(location = 0) out vec4 outColor;

// Material data structure (must match MaterialData in C++)
struct Material {
    vec4 baseColorFactor;       // RGB: base color, A: opacity
    vec4 emissiveFactor;        // RGB: emissive color (additive), A: unused
    float metallicFactor;       // 0.0 = dielectric, 1.0 = metal
    float roughnessFactor;      // 0.0 = smooth, 1.0 = rough
    float normalScale;          // Scale for normal map perturbation
    float occlusionStrength;    // 0.0 = no occlusion, 1.0 = full occlusion
    float alphaCutoff;          // For MASK mode
    int alphaMode;              // 0=OPAQUE, 1=MASK, 2=BLEND
    int doubleSided;            // 0=single-sided, 1=double-sided
    int padding;

    // Texture indices into texture array (-1 = no texture)
    int baseColorTextureIndex;
    int metallicRoughnessTextureIndex;
    int normalTextureIndex;
    int occlusionTextureIndex;

    int emissiveTextureIndex;
    int baseColorTexCoord;
    int metallicRoughnessTexCoord;
    int normalTexCoord;

    int occlusionTexCoord;
    int emissiveTexCoord;
    int padding2;
    int padding3;
};

// ============================================================================
// PBR Functions (GGX/Smith Microfacet BRDF)
// ============================================================================

float chiGGX(float v) {
    return v > 0.0 ? 1.0 : 0.0;
}

// GGX normal distribution function
float D_GGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0001);
}

// Smith's geometric shadowing function
float G_SchlickGGX(float NdotX, float k) {
    return NdotX / (NdotX * (1.0 - k) + k + 1e-6);
}

float G_Smith(float NdotV, float NdotL, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return G_SchlickGGX(NdotV, k) * G_SchlickGGX(NdotL, k);
}

// Fresnel-Schlick approximation
vec3 F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Disney (Burley) diffuse for more accurate energy conservation
float SchlickFresnel(float u) {
    float m = clamp(1.0 - u, 0.0, 1.0);
    float m2 = m * m;
    return m2 * m2 * m;
}

float Diffuse_Burley(float NdotL, float NdotV, float LdotH, float roughness) {
    float fd90 = 0.5 + 2.0 * roughness * LdotH * LdotH;
    float lightScatter = 1.0 + (fd90 - 1.0) * SchlickFresnel(NdotL);
    float viewScatter = 1.0 + (fd90 - 1.0) * SchlickFresnel(NdotV);
    return (lightScatter * viewScatter) / PI;
}

// ============================================================================
// Material Data Extraction
// ============================================================================

Material getMaterial(int index) {
    // Each material now occupies 7 vec4s (112 bytes) in the buffer
    int baseIdx = index * 7;

    Material mat;
    mat.baseColorFactor = materials[baseIdx + 0];
    mat.emissiveFactor = materials[baseIdx + 1];

    vec4 factors = materials[baseIdx + 2];
    mat.metallicFactor = factors.x;
    mat.roughnessFactor = factors.y;
    mat.normalScale = factors.z;
    mat.occlusionStrength = factors.w;

    vec4 alphaData = materials[baseIdx + 3];
    mat.alphaCutoff = alphaData.x;
    mat.alphaMode = floatBitsToInt(alphaData.y);
    mat.doubleSided = floatBitsToInt(alphaData.z);
    mat.padding = floatBitsToInt(alphaData.w);

    // Read texture indices - use floatBitsToInt to reinterpret the float bits as int
    vec4 texIndices1 = materials[baseIdx + 4];
    mat.baseColorTextureIndex = floatBitsToInt(texIndices1.x);
    mat.metallicRoughnessTextureIndex = floatBitsToInt(texIndices1.y);
    mat.normalTextureIndex = floatBitsToInt(texIndices1.z);
    mat.occlusionTextureIndex = floatBitsToInt(texIndices1.w);

    vec4 texIndices2 = materials[baseIdx + 5];
    mat.emissiveTextureIndex = floatBitsToInt(texIndices2.x);
    mat.baseColorTexCoord = floatBitsToInt(texIndices2.y);
    mat.metallicRoughnessTexCoord = floatBitsToInt(texIndices2.z);
    mat.normalTexCoord = floatBitsToInt(texIndices2.w);

    vec4 texIndices3 = materials[baseIdx + 6];
    mat.occlusionTexCoord = floatBitsToInt(texIndices3.x);
    mat.emissiveTexCoord = floatBitsToInt(texIndices3.y);
    mat.padding2 = floatBitsToInt(texIndices3.z);
    mat.padding3 = floatBitsToInt(texIndices3.w);

    return mat;
}

// ============================================================================
// Main Shader
// ============================================================================

void main() {
    // Get material data
    Material mat = getMaterial(pc.materialIndex);

    // Select UV set for base color texture
    vec2 uv = (mat.baseColorTexCoord == 0) ? fragTexCoord0 : fragTexCoord1;

    // Sample base color texture or use factor if no texture
    vec4 baseColor;
    if (mat.baseColorTextureIndex >= 0) {
        // Debug: visualize texture index as color
        // outColor = vec4(float(mat.baseColorTextureIndex) / 10.0, float(mat.metallicRoughnessTextureIndex) / 10.0, float(mat.normalTextureIndex) / 10.0, 1.0);
        // return;
        baseColor = mat.baseColorFactor * texture(textures[mat.baseColorTextureIndex], uv) * fragColor;
    } else {
        baseColor = mat.baseColorFactor * fragColor;
    }

    // Alpha testing for MASK mode
    if (mat.alphaMode == 1 && baseColor.a < mat.alphaCutoff) {
        discard;
    }

    // Sample metallic-roughness texture or use factors
    float roughness, metallic;
    if (mat.metallicRoughnessTextureIndex >= 0) {
        vec2 uvMR = (mat.metallicRoughnessTexCoord == 0) ? fragTexCoord0 : fragTexCoord1;
        // glTF spec: G = roughness, B = metallic
        vec2 metallicRoughness = texture(textures[mat.metallicRoughnessTextureIndex], uvMR).gb;
        roughness = clamp(metallicRoughness.x * mat.roughnessFactor, 0.04, 1.0);
        metallic = clamp(metallicRoughness.y * mat.metallicFactor, 0.0, 1.0);
    } else {
        roughness = clamp(mat.roughnessFactor, 0.04, 1.0);
        metallic = clamp(mat.metallicFactor, 0.0, 1.0);
    }

    // Sample normal map or use vertex normal
    vec3 tangentNormal;
    if (mat.normalTextureIndex >= 0) {
        vec2 uvNormal = (mat.normalTexCoord == 0) ? fragTexCoord0 : fragTexCoord1;
        tangentNormal = texture(textures[mat.normalTextureIndex], uvNormal).xyz * 2.0 - 1.0;
        tangentNormal.xy *= mat.normalScale;
    } else {
        tangentNormal = vec3(0.0, 0.0, 1.0);  // Default tangent-space normal
    }

    // Construct TBN matrix and transform normal to world space
    mat3 TBN = mat3(normalize(fragTangent), normalize(fragBitangent), normalize(fragNormal));
    vec3 N = normalize(TBN * tangentNormal);

    // Sample occlusion map or use full value
    float ao;
    if (mat.occlusionTextureIndex >= 0) {
        vec2 uvAO = (mat.occlusionTexCoord == 0) ? fragTexCoord0 : fragTexCoord1;
        ao = texture(textures[mat.occlusionTextureIndex], uvAO).r;
        ao = 1.0 + mat.occlusionStrength * (ao - 1.0);  // Apply strength
    } else {
        ao = 1.0;  // No occlusion
    }

    // Sample emissive map or use factor
    vec3 emissive;
    if (mat.emissiveTextureIndex >= 0) {
        vec2 uvEmissive = (mat.emissiveTexCoord == 0) ? fragTexCoord0 : fragTexCoord1;
        emissive = mat.emissiveFactor.rgb * texture(textures[mat.emissiveTextureIndex], uvEmissive).rgb;
    } else {
        emissive = mat.emissiveFactor.rgb;
    }

    // ========================================================================
    // Lighting calculations
    // ========================================================================

    vec3 V = normalize(ubo.cameraPosition.xyz - fragWorldPos);
    vec3 L = normalize(ubo.sunDirection.xyz);
    vec3 H = normalize(V + L);
    vec3 R = reflect(-V, N);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    float LdotH = max(dot(L, H), 0.0);

    // Cook-Torrance specular BRDF
    vec3 F0 = mix(vec3(0.04), baseColor.rgb, metallic);  // Dielectric base reflectance = 0.04

    float D = D_GGX(NdotH, roughness);
    float G = G_Smith(NdotV, NdotL, roughness);
    vec3 F = F_Schlick(VdotH, F0);

    vec3 specular = (D * G * F) / max(4.0 * NdotL * NdotV, 0.001);

    // Diffuse BRDF (energy conserving)
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    float Fd = Diffuse_Burley(NdotL, NdotV, LdotH, roughness);
    vec3 diffuse = kD * baseColor.rgb * Fd;

    // Direct lighting
    vec3 Lo = (diffuse + specular) * ubo.sunColor.rgb * NdotL;

    // Image-based lighting (IBL) from environment cubemap
    float maxLod = 12.0;
    float lod = roughness * maxLod;
    vec3 prefilteredColor = textureLod(cubemapTex, vec3(R.y, -R.z, R.x), lod).rgb;

    vec3 F_ibl = F_Schlick(NdotV, F0);
    vec3 specularIBL = prefilteredColor * F_ibl;

    // Ambient lighting
    vec3 ambient = ubo.ambientLight.rgb * baseColor.rgb * ao;

    // Final color
    vec3 color = ambient + Lo + specularIBL + emissive;

    // Output with material alpha for BLEND mode
    float alpha = (mat.alphaMode == 2) ? baseColor.a : 1.0;
    outColor = vec4(color, alpha);
}
