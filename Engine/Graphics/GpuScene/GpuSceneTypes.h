// GPU-side scene structs in scalar layout, read by shaders through buffer device addresses.
// Mirror: ShaderLibrary/Scene.slang. Change both together (field order and types must match exactly).
#pragma once

#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace ghost::graphics::scene {

inline constexpr uint32_t kNoTexture = 0xFFFFFFFFu;

enum MaterialFlags : uint32_t {
    kMaterialAlphaMask = 1u,
    kMaterialDoubleSided = 2u,
};

enum InstanceOverrideFlags : uint32_t {
    kOverrideBaseColor = 1u,
    kOverrideRoughness = 2u,
    kOverrideMetallic = 4u,
    kOverrideEmissive = 8u,
};

enum LightType : uint32_t {
    kLightPoint = 0u,
    kLightSpot = 1u,
};

struct GpuPrimitive {
    uint32_t firstIndex;  // into the global index buffer (indices are relative to firstVertex)
    uint32_t indexCount;
    uint32_t firstVertex; // into the global vertex buffer
    uint32_t material;    // into the global material buffer
};

struct GpuMaterial {
    glm::vec4 baseColorFactor;
    glm::vec3 emissive;
    float metallic;
    float roughness;
    float alphaCutoff;
    float normalScale;
    float transmission;
    uint32_t baseColorTexture;         // bindless index or kNoTexture
    uint32_t metallicRoughnessTexture; // G = roughness, B = metallic
    uint32_t normalTexture;
    uint32_t emissiveTexture;
    uint32_t flags;                    // MaterialFlags
    uint32_t padding[3];
};
static_assert(sizeof(GpuMaterial) == 80);

// One placement of one mesh: an entity x a node of its model. Becomes one TLAS instance in step 7.
struct GpuInstance {
    glm::mat4 world;
    glm::mat4 previousWorld;  // last frame (motion vectors)
    glm::mat4 normalMatrix;   // inverse transpose of world
    uint32_t entityId;        // entt entity + 1; 0 = nothing
    uint32_t firstPrimitive;
    uint32_t primitiveCount;
    uint32_t overrideFlags;   // InstanceOverrideFlags
    glm::vec3 baseColor;
    float roughness;
    glm::vec3 emissive;
    float metallic;
};
static_assert(sizeof(GpuInstance) == 240);

struct GpuDraw {
    uint32_t instance;
    uint32_t primitive;
};

struct GpuLight {
    glm::vec3 position;
    uint32_t type;      // LightType
    glm::vec3 direction; // spot: the direction the light points
    float range;         // 0 = unlimited
    glm::vec3 intensity; // color x candela
    float radius;
    float cosInner;
    float cosOuter;
    uint32_t padding[2];
};
static_assert(sizeof(GpuLight) == 64);

struct FrameConstants {
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewProjection;
    glm::mat4 inverseViewProjection;
    glm::mat4 previousViewProjection;
    glm::vec3 cameraPosition;
    uint32_t frameIndex;
    glm::vec2 renderSize;
    glm::vec2 inverseRenderSize;
    glm::vec3 sunDirection; // towards the sun
    float sunIlluminance;   // lux
    glm::vec3 sunColor;
    float sunAngularRadius; // radians
    glm::vec3 environmentAverage; // average radiance of the HDRI (before intensity)
    float environmentIntensity;
    uint32_t environmentTexture;  // bindless index or kNoTexture
    float exposure;               // linear multiplier from EV100
    uint32_t lightCount;
    uint32_t instanceCount;
    uint64_t vertices;   // buffer device addresses
    uint64_t indices;
    uint64_t primitives;
    uint64_t materials;
    uint64_t instances;
    uint64_t draws;
    uint64_t lights;
    uint32_t hasSun;
    uint32_t padding;
};
static_assert(sizeof(FrameConstants) == 480);

} // namespace ghost::graphics::scene
