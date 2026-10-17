// A model on the CPU: one vertex/index array, primitives (one material each), meshes (groups of primitives, one BLAS
// each on the GPU), nodes (mesh placements inside the model), materials and images. Skinned models also carry the
// full node hierarchy, their skins, per-vertex joint weights and animation clips.
#pragma once

#include "Assets/AnimationData.h"
#include "Assets/ImageData.h"

#include <cstdint>
#include <string>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace ghost::assets {

// 48 bytes, scalar layout: the same struct is read by shaders through a buffer device address (ShaderLibrary/Scene.slang).
struct Vertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f}; // xyz = tangent, w = bitangent sign
    glm::vec2 uv{0.0f};
};
static_assert(sizeof(Vertex) == 48, "Vertex must stay 48 bytes (shader layout)");

enum class AlphaMode { Opaque, Mask, Blend };

// glTF metallic-roughness material. Texture fields index ModelData::images (-1 = none).
struct MaterialData {
    std::string name;
    glm::vec4 baseColorFactor{1.0f};
    int32_t baseColorTexture = -1;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    int32_t metallicRoughnessTexture = -1; // G = roughness, B = metallic (glTF convention)
    int32_t normalTexture = -1;
    float normalScale = 1.0f;
    glm::vec3 emissiveFactor{0.0f};
    float emissiveStrength = 1.0f;
    int32_t emissiveTexture = -1;
    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;
    bool doubleSided = false;
    float transmission = 0.0f;
};

struct PrimitiveData {
    uint32_t firstIndex = 0;  // into ModelData::indices
    uint32_t indexCount = 0;
    uint32_t firstVertex = 0; // indices are relative to this vertex
    uint32_t vertexCount = 0;
    uint32_t material = 0;    // into ModelData::materials
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
};

struct MeshData {
    std::string name;
    std::vector<uint32_t> primitives; // into ModelData::primitives
};

struct NodeData {
    std::string name;
    uint32_t mesh = 0;               // into ModelData::meshes
    glm::mat4 transform{1.0f};       // node -> model space
    int32_t hierarchyNode = -1;      // into ModelData::hierarchy
    int32_t skin = -1;               // into ModelData::skins; -1 = rigid mesh
};

// One node of the glTF node tree (every node, joints included), with its transform relative to its parent.
// A node given as a matrix in the file is decomposed into translation / rotation / scale on import.
struct HierarchyNodeData {
    std::string name;
    int32_t parent = -1;                       // into ModelData::hierarchy; -1 = a root of the model
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    glm::mat4 restGlobal{1.0f};                // node -> model space at rest, computed by cgltf (a test reference)
};

// A glTF skin: the joints a mesh is bound to and, per joint, the matrix that takes a vertex from the mesh's bind
// space into that joint's local space.
struct SkinData {
    std::string name;
    std::vector<uint32_t> joints;               // into ModelData::hierarchy; SkinVertex::joints index this list
    std::vector<glm::mat4> inverseBindMatrices; // parallel to joints
};

// Up to four joint influences per vertex (glTF JOINTS_0 / WEIGHTS_0). Kept apart from Vertex so its 48-byte shader
// layout never changes; parallel to ModelData::vertices when the model has skins, empty otherwise.
struct SkinVertex {
    glm::uvec4 joints{0u};   // into the skin's joint list
    glm::vec4 weights{0.0f}; // sum to 1 for skinned vertices
};
static_assert(sizeof(SkinVertex) == 32, "SkinVertex must stay 32 bytes (shader layout)");

struct ModelData {
    std::string name;
    std::string reference; // how scenes refer to it: "Assets/Models/X/X.gltf" or "procedural:Name"
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<PrimitiveData> primitives;
    std::vector<MeshData> meshes;
    std::vector<NodeData> nodes;
    std::vector<MaterialData> materials;
    std::vector<ImageData> images;
    std::vector<HierarchyNodeData> hierarchy; // every node of the file (filled for glTF models)
    std::vector<SkinData> skins;
    std::vector<SkinVertex> skinVertices;     // parallel to vertices, or empty
    std::vector<AnimationData> animations;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
};

} // namespace ghost::assets
