#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragTangent;

layout(set = 0, binding = 0) uniform UniformBufferObject {
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

layout(set = 0, binding = 1) uniform sampler2D baseTex;
layout(set = 0, binding = 2) uniform sampler2D normalTex;
layout(set = 0, binding = 3) uniform sampler2D metalRoughnessTex;
layout(set = 0, binding = 4) uniform sampler2D aoTex;
layout(set = 0, binding = 5) uniform sampler2D emissiveTex;
layout(set = 0, binding = 6) uniform samplerCube cubemapTex;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

// --- GGX helpers ---
float chiGGX(float v)
{
    return v > 0 ? 1.0f : 0.0f;
}

float D_GGX(float NdotH, float a2) {
    float NdotH2 = NdotH * NdotH;
    float den = NdotH2 * a2 + (1 - NdotH2);
    return (chiGGX(NdotH) * a2) / ( PI * den * den );
}
float G_SchlickGGX(float NdotX, float k) {
    return NdotX / (NdotX * (1.0 - k) + k + 1e-6);
}
float G_Smith(float NdotV, float NdotL, float k) {
    return G_SchlickGGX(NdotV, k) * G_SchlickGGX(NdotL, k);
}
vec3  F_Schlick(float cosTheta, vec3 F0) {
    float p = pow(1.0 - cosTheta, 5.0);
    return F0 + (1.0 - F0) * p;
}

// --- Disney (Burley) diffuse ------------------------------------
// Reference: "Physically-Based Shading at Disney" (2012)
float SchlickFresnel(float u) 
{   float m = clamp(1.0 - u, 0.0, 1.0); 
    float m2 = m*m; return m2*m2*m; 
}

float Diffuse_Burley(float NdotL, float NdotV, float LdotH, float linearRoughness)
{
    // Fd90 term increases retro-reflection with roughness
    float fd90 = 0.5 + 2.0 * linearRoughness * LdotH * LdotH;
    float lightScatter = 1.0 + (fd90 - 1.0) * SchlickFresnel(NdotL);
    float viewScatter  = 1.0 + (fd90 - 1.0) * SchlickFresnel(NdotV);
    return (lightScatter * viewScatter) * (1.0 / PI);
}

void main() {
    // --- Material maps ---
    vec3  base     = texture(baseTex,           fragTexCoord).rgb * fragColor;
    vec3  mr       = texture(metalRoughnessTex, fragTexCoord).rgb;
    float rough    = clamp(mr.g, 0.04, 1.0);
    float metal    = clamp(mr.b, 0.0, 1.0);
    float ao       = texture(aoTex,             fragTexCoord).r;
    vec3  emissive = texture(emissiveTex,       fragTexCoord).rgb;

    // --- World-space TBN & normal map ---
    vec3 Nw = normalize(fragNormal);
    vec3 Tw = normalize(fragTangent);
    Tw = normalize(Tw - Nw * dot(Nw, Tw));
    vec3 Bw = normalize(cross(Nw, Tw));
    mat3 TBN = mat3(Tw, Bw, Nw);

    vec3 nTan = texture(normalTex, fragTexCoord).xyz * 2.0 - 1.0;
    vec3 N    = normalize(TBN * nTan);   // final world-space normal

    // --- Lighting vectors (world-space) ---
    vec3 L = normalize(ubo.sunDirection.xyz);
    vec3 V = normalize(ubo.cameraPosition.xyz - fragPos);
    vec3 H = normalize(V + L);
    vec3 R = reflect(V, N);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    float LdotH = max(dot(L, H), 0.0);;

    // --- Cook–Torrance (direct lighting) ---
    vec3  F0 = mix(vec3(0.04), base, metal);
    float a  = rough * rough;
    float k  = ((rough + 1.0) * (rough + 1.0)) / 8.0;

    float D = D_GGX(NdotH, a);
    float G = G_Smith(NdotV, NdotL, k);
    vec3  F = F_Schlick(VdotH, F0); //?

    vec3  spec = (D * G) * F / max(4.0 * NdotL * NdotV, 1e-5);
    
    // Energy-conserving diffuse
    vec3  kd   = (1.0 - F) * (1.0 - metal);
    float Fd   = Diffuse_Burley(NdotL, NdotV, LdotH, rough);
    vec3  diff = kd * base * Fd;

    vec3 Lo = (diff + spec) * (ubo.sunColor.rgb * NdotL);

    // --- Specular IBL from prefiltered environment cubemap ---
    // Replace 5.0 with (textureQueryLevels(cubemapTex)-1) passed via UBO or push constant for accuracy
    float maxLod = 12.0;
    float lod    = clamp(rough, 0.0, 1.0) * maxLod;
    vec3 dir= vec3(R.y,-R.z,R.x);
    vec3 prefiltered = textureLod(cubemapTex, dir, lod).rgb;

    // Approximate split-sum when BRDF LUT is not available
    vec3  F_ibl   = F_Schlick(NdotV, F0);
    vec3  specIBL = prefiltered * F_ibl;

    // --- Diffuse ambient placeholder (replace with irradiance cubemap when available) ---
    vec3 ambient = ubo.ambientLight.rgb * base * ao;

    vec3 color = ambient + Lo + specIBL + emissive;
    //color = vec3(ao);
    //color= N;
    outColor = vec4(color, 1.0);
}