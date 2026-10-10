// The scene as the GPU sees it: global vertex/index/primitive/material buffers, bindless textures, the environment map,
// and per-frame instances, draws, lights and frame constants. Filled with plain data (no ECS types): the World's
// GpuSceneExtractionSystem feeds it, techniques read it through ShaderLibrary/Scene.slang.
#pragma once

#include "Assets/ModelData.h"
#include "Graphics/GpuScene/GpuSceneTypes.h"
#include "Graphics/GpuScene/TextureUploader.h"
#include "Graphics/Vulkan/GpuBuffer.h"
#include "Graphics/Vulkan/ImmediateSubmit.h"

#include <array>
#include <cstdint>
#include <vector>

#include <glm/mat4x4.hpp>
#include <volk.h>

namespace ghost::graphics::vulkan {
class Device;
class BindlessDescriptors;
class DeletionQueue;
} // namespace ghost::graphics::vulkan

namespace ghost::graphics::scene {

class GpuScene {
public:
    using ModelId = uint32_t;
    static constexpr ModelId kInvalidModel = UINT32_MAX;
    static constexpr uint32_t kFrameSlots = 2;
    static constexpr VkDeviceSize kVertexStride = sizeof(assets::Vertex); // position is the first member (BLAS input)

    // Raster draws are bucketed by the pipeline they need.
    enum class DrawCategory : uint32_t { Opaque = 0, DoubleSided = 1, AlphaMasked = 2, Count = 3 };

    struct InstanceInput {
        ModelId model = kInvalidModel;
        glm::mat4 world{1.0f};
        glm::mat4 previousWorld{1.0f};
        uint32_t entityId = 0; // entity + 1
        uint32_t overrideFlags = 0;
        glm::vec3 baseColor{1.0f};
        float roughness = 0.5f;
        float metallic = 0.0f;
        glm::vec3 emissive{0.0f};
    };

    struct SunInput {
        bool enabled = false;
        glm::vec3 directionToSun{0.0f, 1.0f, 0.0f};
        float illuminance = 0.0f;
        glm::vec3 color{1.0f};
        float angularRadius = 0.0f;
    };

    struct CameraInput {
        glm::vec3 position{0.0f};
        glm::vec3 forward{0.0f, 0.0f, -1.0f};
        float fovYDeg = 60.0f;
        float nearPlane = 0.05f;
    };

    struct DrawCommand {
        uint32_t indexCount = 0;
        uint32_t firstIndex = 0;
        int32_t vertexOffset = 0;
        uint32_t drawIndex = 0; // firstInstance: the shader reads draws[drawIndex]
    };

    GpuScene() = default;
    ~GpuScene();
    GpuScene(const GpuScene&) = delete;
    GpuScene& operator=(const GpuScene&) = delete;

    void initialize(const vulkan::Device& device, vulkan::BindlessDescriptors& bindless, vulkan::DeletionQueue& deletionQueue);
    void shutdown();

    // Load-time uploads (blocking). Geometry goes into the global buffers; textures become bindless.
    ModelId uploadModel(const assets::ModelData& model);
    // Forgets every model (their ids become invalid); GPU memory is released once in-flight frames finish.
    void clearModels();
    // nullptr = no HDRI: shaders fall back to the procedural sky (ShaderLibrary/Environment.slang).
    void setEnvironment(const assets::ImageData* image, float intensity);
    void setEnvironmentIntensity(float intensity);

    // Per-frame scene description (CPU side; copied to the GPU in prepareFrame).
    void setInstances(const std::vector<InstanceInput>& instances);
    void setLights(const SunInput& sun, std::vector<GpuLight> lights);
    void setCamera(const CameraInput& camera, bool resetHistory);
    void setExposure(float linearExposure) { m_exposure = linearExposure; }
    float exposure() const { return m_exposure; }

    // Renderer, once the frame slot is free: writes this frame's buffers and constants.
    void prepareFrame(uint32_t slot, uint64_t frameIndex, VkExtent2D renderSize, uint64_t lastSubmittedFrame);
    VkDeviceAddress frameConstantsAddress() const;
    const FrameConstants& frameConstants() const { return m_constants; }
    VkBuffer indexBuffer() const { return m_indexBuffer.handle(); }
    const std::vector<DrawCommand>& drawCommands(DrawCategory category) const { return m_drawCommands[static_cast<uint32_t>(category)]; }
    bool hasGeometry() const { return !m_gpuDraws.empty(); }

    // For acceleration structures: every mesh of every model (one BLAS each), as a range of primitives (one BLAS
    // geometry each), and for every instance the mesh it places.
    struct MeshRange {
        uint32_t firstPrimitive = 0;
        uint32_t primitiveCount = 0;
    };
    struct NodeRecord {
        uint32_t mesh = 0; // into the model's meshes
        glm::mat4 transform{1.0f};
    };
    struct GpuModel {
        std::vector<uint32_t> meshes; // global mesh indices (into meshes())
        std::vector<NodeRecord> nodes;
    };
    const std::vector<GpuModel>& models() const { return m_models; }
    const std::vector<MeshRange>& meshes() const { return m_meshes; }
    const std::vector<GpuPrimitive>& primitives() const { return m_primitives; }
    const std::vector<uint32_t>& primitiveVertexCounts() const { return m_primitiveVertexCounts; } // parallel to primitives()
    const std::vector<GpuMaterial>& materials() const { return m_materials; }
    const std::vector<GpuInstance>& instances() const { return m_gpuInstances; }
    const std::vector<uint32_t>& instanceMeshes() const { return m_instanceMeshes; } // parallel to instances()
    VkDeviceAddress vertexBufferAddress() const { return m_vertexBuffer.address(); }
    VkDeviceAddress indexBufferAddress() const { return m_indexBuffer.address(); }
    uint32_t vertexCount() const { return static_cast<uint32_t>(m_vertices.size()); }
    // Increments whenever geometry changes (BLAS rebuild trigger).
    uint64_t geometryRevision() const { return m_geometryRevision; }
    // Increments whenever anything that changes the rendered image changes (geometry, instances, lights, environment);
    // the camera is not included. Accumulating passes restart when it moves.
    uint64_t sceneRevision() const { return m_sceneRevision; }
    // Increments whenever the sun or the point/spot light list changes (e.g. rebuild a light sampling table).
    uint64_t lightRevision() const { return m_lightRevision; }

private:
    struct FrameSlot {
        vulkan::GpuBuffer constants;
        vulkan::GpuBuffer instances;
        vulkan::GpuBuffer draws;
        vulkan::GpuBuffer lights;
    };

    void rebuildGeometryBuffers();
    void retireTextures(std::vector<TextureUploader::Texture>& textures);
    void ensureCapacity(vulkan::GpuBuffer& buffer, size_t bytes, const char* name);

    const vulkan::Device* m_device = nullptr;
    vulkan::BindlessDescriptors* m_bindless = nullptr;
    vulkan::DeletionQueue* m_deletionQueue = nullptr;
    vulkan::ImmediateSubmit m_submit;

    // Geometry (CPU copies kept so the global buffers can be rebuilt when a model is added).
    std::vector<assets::Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<GpuPrimitive> m_primitives;
    std::vector<uint32_t> m_primitiveVertexCounts;
    std::vector<GpuMaterial> m_materials;
    std::vector<GpuModel> m_models;
    std::vector<MeshRange> m_meshes;
    std::vector<TextureUploader::Texture> m_textures;
    vulkan::GpuBuffer m_vertexBuffer;
    vulkan::GpuBuffer m_indexBuffer;
    vulkan::GpuBuffer m_primitiveBuffer;
    vulkan::GpuBuffer m_materialBuffer;
    uint64_t m_geometryRevision = 0;
    uint64_t m_sceneRevision = 0;
    uint64_t m_lightRevision = 0;

    std::vector<TextureUploader::Texture> m_environment;
    glm::vec3 m_environmentAverage{0.0f};
    float m_environmentIntensity = 0.0f;

    // This frame.
    std::vector<GpuInstance> m_gpuInstances;
    std::vector<uint32_t> m_instanceMeshes;
    std::vector<GpuDraw> m_gpuDraws;
    std::array<std::vector<DrawCommand>, static_cast<size_t>(DrawCategory::Count)> m_drawCommands;
    std::vector<GpuLight> m_lights;
    SunInput m_sun;
    CameraInput m_camera;
    bool m_resetHistory = true;
    glm::mat4 m_previousViewProjection{1.0f};
    float m_exposure = 1.0f;
    FrameConstants m_constants{};
    std::array<FrameSlot, kFrameSlots> m_slots;
    uint32_t m_slot = 0;
    uint64_t m_lastSubmittedFrame = 0;
};

} // namespace ghost::graphics::scene
