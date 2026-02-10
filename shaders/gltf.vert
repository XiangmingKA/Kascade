#version 450

// ============================================================================
// glTF Vertex Shader
// Supports glTF 2.0 vertex attributes and node transforms
// ============================================================================

// Set 0: Per-frame data (camera, lights)
// NOTE: Must match UniformBufferObject in Application.cpp exactly
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;          // Not used in glTF (per-node transforms instead)
    mat4 view;
    mat4 proj;
    mat4 normalMat;      // Not used in glTF
    vec4 cameraPosition;
    vec4 viewDirection;
    vec4 sunDirection;
    vec4 sunColor;
    vec4 ambientLight;
} ubo;

// Set 1: Per-model data (transforms for all nodes)
layout(set = 1, binding = 0) readonly buffer TransformBuffer {
    mat4 nodeTransforms[];
};

// Push constants for node index
layout(push_constant) uniform PushConstants {
    int nodeIndex;
    int materialIndex;
} pc;

// Vertex inputs matching GltfVertex structure
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord0;
layout(location = 3) in vec2 inTexCoord1;
layout(location = 4) in vec4 inColor;
layout(location = 5) in vec4 inTangent;      // xyz = tangent, w = handedness
layout(location = 6) in vec4 inJoints;       // For skinning (future)
layout(location = 7) in vec4 inWeights;      // For skinning (future)

// Outputs to fragment shader
layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragTangent;
layout(location = 3) out vec3 fragBitangent;
layout(location = 4) out vec2 fragTexCoord0;
layout(location = 5) out vec2 fragTexCoord1;
layout(location = 6) out vec4 fragColor;

void main() {
    // Get node transform from storage buffer
    mat4 modelMatrix = nodeTransforms[pc.nodeIndex];
    mat4 normalMatrix = transpose(inverse(modelMatrix));

    // Transform position to world space
    vec4 worldPos = modelMatrix * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;

    // Transform position to clip space
    gl_Position = ubo.proj * ubo.view * worldPos;

    // Transform normal and tangent to world space
    fragNormal = normalize((normalMatrix * vec4(inNormal, 0.0)).xyz);

    // Handle tangent with handedness for bitangent calculation
    vec3 tangent = normalize((normalMatrix * vec4(inTangent.xyz, 0.0)).xyz);
    fragTangent = normalize(tangent - fragNormal * dot(fragNormal, tangent)); // Gram-Schmidt
    fragBitangent = cross(fragNormal, fragTangent) * inTangent.w;

    // Pass through texture coordinates and color
    fragTexCoord0 = inTexCoord0;
    fragTexCoord1 = inTexCoord1;
    fragColor = inColor;
}
