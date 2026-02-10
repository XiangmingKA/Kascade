#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normalMat;
    vec4 cameraPosition;
    vec4 viewDirection;
    vec4 sunDirection;
    vec4 sunColor;
    vec4 ambientLight;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragNormals;
layout(location = 4) out vec3 fragTangent;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragPos = vec3(ubo.model * vec4(inPosition, 1.0));
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    fragNormals = (ubo.normalMat * vec4(inNormal, 0.0)).xyz;
    fragTangent = (ubo.normalMat * vec4(inTangent, 0.0)).xyz;
}