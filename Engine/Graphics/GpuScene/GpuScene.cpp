// GpuScene: model upload into global buffers, per-frame instance/draw/light expansion, camera matrices (reverse-Z).
#include "Graphics/GpuScene/GpuScene.h"

#include "Assets/ImageLoader.h"
#include "Core/Log.h"
#include "Core/Paths.h"
#include "Graphics/Vulkan/BindlessDescriptors.h"
#include "Graphics/Vulkan/DeletionQueue.h"
#include "Graphics/Vulkan/Device.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <memory>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ghost::graphics::scene {

namespace {

// Reverse-Z, infinite far plane, Vulkan clip space (y down): depth = near / viewDistance, 1 at the near plane, 0 at infinity.
glm::mat4 reverseZPerspective(float fovYRadians, float aspect, float nearPlane) {
    const float f = 1.0f / std::tan(fovYRadians * 0.5f);
    glm::mat4 projection(0.0f);
    projection[0][0] = f / aspect;
    projection[1][1] = -f;
    projection[2][3] = -1.0f;
    projection[3][2] = nearPlane;
    return projection;
}

uint32_t textureIndex(int32_t local, const std::vector<uint32_t>& map) {
    return (local >= 0 && static_cast<size_t>(local) < map.size()) ? map[static_cast<size_t>(local)] : kNoTexture;
}

} // namespace

GpuScene::~GpuScene() {
    shutdown();
}

void GpuScene::initialize(const vulkan::Device& device, vulkan::BindlessDescriptors& bindless, vulkan::DeletionQueue& deletionQueue) {
    m_device = &device;
    m_bindless = &bindless;
    m_deletionQueue = &deletionQueue;
    m_submit.create(device);
    loadBlueNoise();
}

void GpuScene::loadBlueNoise() {
    // Christoph Peters' 128x128 RGBA blue noise (CC0), fetched by run.bat through Content/AssetManifest.json.
    const std::filesystem::path path = core::Paths::content() / "Assets" / "BlueNoise" / "LDR_RGBA_0.png";
    assets::ImageData image;
    std::string error;
    if (!assets::ImageLoader::loadFile(path, assets::ImageData::Format::Rgba8, image, error)) {
        core::Log::warning("GPU scene: no blue noise texture ({}); blueNoise() falls back to white noise", error);
        return;
    }
    const assets::ImageData* images[] = {&image};
    m_blueNoise = TextureUploader::upload(*m_device, *m_bindless, m_submit, images);
}

void GpuScene::shutdown() {
    if (!m_device) {
        return;
    }
    // The caller waited for the GPU: release immediately.
    for (auto* list : {&m_textures, &m_environment, &m_blueNoise}) {
        for (TextureUploader::Texture& texture : *list) {
            if (texture.bindlessIndex != kNoTexture) {
                m_bindless->freeSampledImage(texture.bindlessIndex);
            }
            texture.image.destroy();
        }
        list->clear();
    }
    m_vertexBuffer.destroy();
    m_indexBuffer.destroy();
    m_primitiveBuffer.destroy();
    m_materialBuffer.destroy();
    for (FrameSlot& slot : m_slots) {
        slot = FrameSlot{};
    }
    m_submit.destroy();
    m_device = nullptr;
}

GpuScene::ModelId GpuScene::uploadModel(const assets::ModelData& model) {
    const uint32_t vertexBase = static_cast<uint32_t>(m_vertices.size());
    const uint32_t indexBase = static_cast<uint32_t>(m_indices.size());
    const uint32_t primitiveBase = static_cast<uint32_t>(m_primitives.size());
    const uint32_t materialBase = static_cast<uint32_t>(m_materials.size());

    std::vector<const assets::ImageData*> images;
    for (const assets::ImageData& image : model.images) {
        images.push_back(&image);
    }
    std::vector<TextureUploader::Texture> uploaded = TextureUploader::upload(*m_device, *m_bindless, m_submit, images);
    std::vector<uint32_t> textureMap;
    size_t textureBytes = 0;
    for (size_t i = 0; i < uploaded.size(); ++i) {
        textureMap.push_back(uploaded[i].bindlessIndex);
        textureBytes += model.images[i].pixels.size();
        m_textures.push_back(std::move(uploaded[i]));
    }

    for (const assets::MaterialData& material : model.materials) {
        GpuMaterial gpu{};
        gpu.baseColorFactor = material.baseColorFactor;
        gpu.emissive = material.emissiveFactor * material.emissiveStrength;
        gpu.metallic = material.metallicFactor;
        gpu.roughness = material.roughnessFactor;
        gpu.alphaCutoff = material.alphaCutoff;
        gpu.normalScale = material.normalScale;
        gpu.transmission = material.transmission;
        gpu.baseColorTexture = textureIndex(material.baseColorTexture, textureMap);
        gpu.metallicRoughnessTexture = textureIndex(material.metallicRoughnessTexture, textureMap);
        gpu.normalTexture = textureIndex(material.normalTexture, textureMap);
        gpu.emissiveTexture = textureIndex(material.emissiveTexture, textureMap);
        // Deferred shading can't blend: blended materials are treated as alpha-tested.
        gpu.flags = (material.alphaMode != assets::AlphaMode::Opaque ? kMaterialAlphaMask : 0u) |
                    (material.doubleSided ? kMaterialDoubleSided : 0u);
        m_materials.push_back(gpu);
    }
    for (const assets::PrimitiveData& primitive : model.primitives) {
        m_primitives.push_back({indexBase + primitive.firstIndex, primitive.indexCount, vertexBase + primitive.firstVertex, materialBase + primitive.material});
        m_primitiveVertexCounts.push_back(primitive.vertexCount);
    }
    m_vertices.insert(m_vertices.end(), model.vertices.begin(), model.vertices.end());
    m_indices.insert(m_indices.end(), model.indices.begin(), model.indices.end());

    GpuModel gpuModel;
    for (const assets::MeshData& mesh : model.meshes) {
        // Importers lay a mesh's primitives out contiguously, so a mesh is one primitive range.
        const uint32_t first = mesh.primitives.empty() ? 0u : mesh.primitives.front();
        gpuModel.meshes.push_back(static_cast<uint32_t>(m_meshes.size()));
        m_meshes.push_back({primitiveBase + first, static_cast<uint32_t>(mesh.primitives.size())});
    }
    for (const assets::NodeData& node : model.nodes) {
        gpuModel.nodes.push_back({node.mesh, node.transform});
    }
    m_models.push_back(std::move(gpuModel));
    rebuildGeometryBuffers();

    core::Log::info("GPU scene: uploaded {} ({} textures, {:.0f} MB of texels)", model.reference, model.images.size(),
                    static_cast<double>(textureBytes) / (1024.0 * 1024.0));
    return static_cast<ModelId>(m_models.size() - 1);
}

void GpuScene::rebuildGeometryBuffers() {
    auto retire = [this](vulkan::GpuBuffer& buffer) {
        if (buffer.valid()) {
            auto old = std::make_shared<vulkan::GpuBuffer>(std::move(buffer));
            m_deletionQueue->push(m_lastSubmittedFrame, [old] { old->destroy(); });
        }
    };
    retire(m_vertexBuffer);
    retire(m_indexBuffer);
    retire(m_primitiveBuffer);
    retire(m_materialBuffer);

    constexpr VkBufferUsageFlags kGeometry = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    struct Upload {
        vulkan::GpuBuffer* target;
        const void* data;
        size_t bytes;
        VkBufferUsageFlags usage;
        const char* name;
    };
    const Upload uploads[] = {
        {&m_vertexBuffer, m_vertices.data(), m_vertices.size() * sizeof(assets::Vertex), kGeometry, "Scene.Vertices"},
        {&m_indexBuffer, m_indices.data(), m_indices.size() * sizeof(uint32_t), kGeometry | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, "Scene.Indices"},
        {&m_primitiveBuffer, m_primitives.data(), m_primitives.size() * sizeof(GpuPrimitive), kGeometry, "Scene.Primitives"},
        {&m_materialBuffer, m_materials.data(), m_materials.size() * sizeof(GpuMaterial), kGeometry, "Scene.Materials"},
    };
    size_t total = 0;
    for (const Upload& upload : uploads) {
        total += (upload.bytes + 15) & ~size_t{15};
    }
    vulkan::GpuBuffer staging;
    staging.create(*m_device, {std::max<VkDeviceSize>(total, 16), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vulkan::GpuBuffer::Memory::Upload, "GeometryStaging"});
    size_t offset = 0;
    std::vector<size_t> offsets;
    for (const Upload& upload : uploads) {
        upload.target->create(*m_device, {std::max<VkDeviceSize>(upload.bytes, 16), upload.usage, vulkan::GpuBuffer::Memory::GpuOnly, upload.name});
        std::memcpy(static_cast<uint8_t*>(staging.mapped()) + offset, upload.data, upload.bytes);
        offsets.push_back(offset);
        offset += (upload.bytes + 15) & ~size_t{15};
    }
    m_submit.run([&](VkCommandBuffer cmd) {
        for (size_t i = 0; i < std::size(uploads); ++i) {
            if (uploads[i].bytes == 0) {
                continue;
            }
            const VkBufferCopy copy{offsets[i], 0, uploads[i].bytes};
            vkCmdCopyBuffer(cmd, staging.handle(), uploads[i].target->handle(), 1, &copy);
        }
        // Make the copies visible to every later use (shaders, index fetch, acceleration structure builds).
        VkMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
        VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependency.memoryBarrierCount = 1;
        dependency.pMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(cmd, &dependency);
    });
    ++m_geometryRevision;
    ++m_sceneRevision;
}

void GpuScene::retireTextures(std::vector<TextureUploader::Texture>& textures) {
    for (TextureUploader::Texture& texture : textures) {
        auto old = std::make_shared<vulkan::GpuImage>(std::move(texture.image));
        const uint32_t index = texture.bindlessIndex;
        vulkan::BindlessDescriptors* bindless = m_bindless;
        // Frames still in flight may sample the image through its bindless slot: free both once they finish.
        m_deletionQueue->push(m_lastSubmittedFrame, [old, index, bindless] {
            old->destroy();
            if (index != kNoTexture) {
                bindless->freeSampledImage(index);
            }
        });
    }
    textures.clear();
}

void GpuScene::clearModels() {
    retireTextures(m_textures);
    m_vertices.clear();
    m_indices.clear();
    m_primitives.clear();
    m_primitiveVertexCounts.clear();
    m_materials.clear();
    m_models.clear();
    m_meshes.clear();
    m_gpuInstances.clear();
    m_instanceMeshes.clear();
    m_gpuDraws.clear();
    for (std::vector<DrawCommand>& commands : m_drawCommands) {
        commands.clear();
    }
    rebuildGeometryBuffers();
}

void GpuScene::setEnvironmentIntensity(float intensity) {
    if (intensity != m_environmentIntensity) {
        m_environmentIntensity = intensity;
        ++m_sceneRevision;
    }
}

void GpuScene::setEnvironment(const assets::ImageData* image, float intensity) {
    retireTextures(m_environment);
    m_environmentAverage = glm::vec3(0.0f);
    m_environmentIntensity = intensity;
    ++m_sceneRevision;
    if (!image || !image->valid()) {
        return;
    }
    const assets::ImageData* images[] = {image};
    m_environment = TextureUploader::upload(*m_device, *m_bindless, m_submit, images);

    // Average radiance over the sphere (rows weighted by solid angle): the placeholder ambient term.
    const float* texels = reinterpret_cast<const float*>(image->pixels.data());
    glm::dvec3 sum(0.0);
    double weightSum = 0.0;
    for (uint32_t y = 0; y < image->height; ++y) {
        const double weight = std::sin(3.14159265358979 * (y + 0.5) / image->height);
        for (uint32_t x = 0; x < image->width; ++x) {
            const float* t = texels + (static_cast<size_t>(y) * image->width + x) * 4;
            sum += glm::dvec3(t[0], t[1], t[2]) * weight;
            weightSum += weight;
        }
    }
    m_environmentAverage = glm::vec3(sum / weightSum);
    const float luminance = glm::dot(m_environmentAverage, glm::vec3(0.2126f, 0.7152f, 0.0722f));
    core::Log::info("GPU scene: environment {} ({}x{}), average luminance {:.3f} x intensity {} = {:.0f} nits", image->name, image->width,
                    image->height, luminance, intensity, luminance * intensity);
}

void GpuScene::setInstances(const std::vector<InstanceInput>& instances) {
    const std::vector<GpuInstance> previous = std::move(m_gpuInstances);
    m_gpuInstances.clear();
    m_instanceMeshes.clear();
    m_gpuDraws.clear();
    std::array<std::vector<GpuDraw>, static_cast<size_t>(DrawCategory::Count)> buckets;
    for (const InstanceInput& input : instances) {
        if (input.model >= m_models.size()) {
            continue;
        }
        const GpuModel& model = m_models[input.model];
        for (const NodeRecord& node : model.nodes) {
            const uint32_t meshIndex = model.meshes[node.mesh];
            const MeshRange& mesh = m_meshes[meshIndex];
            GpuInstance instance{};
            instance.world = input.world * node.transform;
            instance.previousWorld = input.previousWorld * node.transform;
            instance.normalMatrix = glm::inverseTranspose(instance.world);
            instance.entityId = input.entityId;
            instance.firstPrimitive = mesh.firstPrimitive;
            instance.primitiveCount = mesh.primitiveCount;
            instance.overrideFlags = input.overrideFlags;
            instance.baseColor = input.baseColor;
            instance.roughness = input.roughness;
            instance.emissive = input.emissive;
            instance.metallic = input.metallic;
            const uint32_t instanceIndex = static_cast<uint32_t>(m_gpuInstances.size());
            m_gpuInstances.push_back(instance);
            m_instanceMeshes.push_back(meshIndex);
            for (uint32_t p = 0; p < mesh.primitiveCount; ++p) {
                const uint32_t primitive = mesh.firstPrimitive + p;
                const uint32_t flags = m_materials[m_primitives[primitive].material].flags;
                const DrawCategory category = (flags & kMaterialAlphaMask)      ? DrawCategory::AlphaMasked
                                              : (flags & kMaterialDoubleSided) ? DrawCategory::DoubleSided
                                                                                : DrawCategory::Opaque;
                buckets[static_cast<size_t>(category)].push_back({instanceIndex, primitive});
            }
        }
    }
    for (size_t c = 0; c < buckets.size(); ++c) {
        m_drawCommands[c].clear();
        for (const GpuDraw& draw : buckets[c]) {
            const GpuPrimitive& primitive = m_primitives[draw.primitive];
            m_drawCommands[c].push_back({primitive.indexCount, primitive.firstIndex, static_cast<int32_t>(primitive.firstVertex),
                                         static_cast<uint32_t>(m_gpuDraws.size())});
            m_gpuDraws.push_back(draw);
        }
    }
    // GpuInstance has no padding, so a byte compare is exact.
    if (previous.size() != m_gpuInstances.size() ||
        std::memcmp(previous.data(), m_gpuInstances.data(), previous.size() * sizeof(GpuInstance)) != 0) {
        ++m_sceneRevision;
    }
}

void GpuScene::setLights(const SunInput& sun, std::vector<GpuLight> lights) {
    const bool sunChanged = sun.enabled != m_sun.enabled || sun.directionToSun != m_sun.directionToSun ||
                            sun.illuminance != m_sun.illuminance || sun.color != m_sun.color || sun.angularRadius != m_sun.angularRadius;
    // GpuLight's padding is zero-initialized by the extraction system, so a byte compare is exact.
    const bool lightsChanged =
        lights.size() != m_lights.size() || std::memcmp(lights.data(), m_lights.data(), lights.size() * sizeof(GpuLight)) != 0;
    if (sunChanged || lightsChanged) {
        ++m_sceneRevision;
        ++m_lightRevision;
    }
    m_sun = sun;
    m_lights = std::move(lights);
}

void GpuScene::setCamera(const CameraInput& camera, bool resetHistory) {
    m_camera = camera;
    m_resetHistory = m_resetHistory || resetHistory;
}

void GpuScene::ensureCapacity(vulkan::GpuBuffer& buffer, size_t bytes, const char* name) {
    if (buffer.valid() && buffer.size() >= bytes) {
        return;
    }
    // This slot's previous frame has finished on the GPU (the renderer waited), so the old buffer can go now.
    buffer.create(*m_device, {std::max<VkDeviceSize>(bytes * 3 / 2, 256), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vulkan::GpuBuffer::Memory::Upload, name});
}

void GpuScene::prepareFrame(uint32_t slot, uint64_t frameIndex, VkExtent2D renderSize, uint64_t lastSubmittedFrame) {
    m_slot = slot % kFrameSlots;
    m_lastSubmittedFrame = lastSubmittedFrame;
    FrameSlot& frame = m_slots[m_slot];
    ensureCapacity(frame.constants, sizeof(FrameConstants), "Scene.FrameConstants");
    ensureCapacity(frame.instances, m_gpuInstances.size() * sizeof(GpuInstance), "Scene.Instances");
    ensureCapacity(frame.draws, m_gpuDraws.size() * sizeof(GpuDraw), "Scene.Draws");
    ensureCapacity(frame.lights, m_lights.size() * sizeof(GpuLight), "Scene.Lights");
    std::memcpy(frame.instances.mapped(), m_gpuInstances.data(), m_gpuInstances.size() * sizeof(GpuInstance));
    std::memcpy(frame.draws.mapped(), m_gpuDraws.data(), m_gpuDraws.size() * sizeof(GpuDraw));
    std::memcpy(frame.lights.mapped(), m_lights.data(), m_lights.size() * sizeof(GpuLight));

    const float aspect = static_cast<float>(renderSize.width) / static_cast<float>(std::max(renderSize.height, 1u));
    const glm::mat4 view = glm::lookAt(m_camera.position, m_camera.position + m_camera.forward, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 projection = reverseZPerspective(glm::radians(m_camera.fovYDeg), aspect, m_camera.nearPlane);
    const glm::mat4 viewProjection = projection * view;
    if (m_resetHistory) {
        m_previousViewProjection = viewProjection; // no motion on the first frame after a scene load or teleport
        m_resetHistory = false;
    }

    FrameConstants& c = m_constants;
    c.view = view;
    c.projection = projection;
    c.viewProjection = viewProjection;
    c.inverseViewProjection = glm::inverse(viewProjection);
    c.previousViewProjection = m_previousViewProjection;
    c.cameraPosition = m_camera.position;
    c.frameIndex = static_cast<uint32_t>(frameIndex);
    c.renderSize = glm::vec2(renderSize.width, renderSize.height);
    c.inverseRenderSize = 1.0f / c.renderSize;
    c.sunDirection = glm::normalize(m_sun.directionToSun);
    c.sunIlluminance = m_sun.enabled ? m_sun.illuminance : 0.0f;
    c.sunColor = m_sun.color;
    c.sunAngularRadius = m_sun.angularRadius;
    c.hasSun = m_sun.enabled ? 1u : 0u;
    c.tlasIndex = m_slot; // SceneAccelerationStructures keeps one TLAS per frame slot at this bindless index
    c.environmentAverage = m_environmentAverage;
    c.environmentIntensity = m_environmentIntensity;
    c.environmentTexture = m_environment.empty() ? kNoTexture : m_environment.front().bindlessIndex;
    c.blueNoiseTexture = m_blueNoise.empty() ? kNoTexture : m_blueNoise.front().bindlessIndex;
    c.exposure = m_exposure;
    c.lightCount = static_cast<uint32_t>(m_lights.size());
    c.instanceCount = static_cast<uint32_t>(m_gpuInstances.size());
    c.vertices = m_vertexBuffer.address();
    c.indices = m_indexBuffer.address();
    c.primitives = m_primitiveBuffer.address();
    c.materials = m_materialBuffer.address();
    c.instances = frame.instances.address();
    c.draws = frame.draws.address();
    c.lights = frame.lights.address();
    std::memcpy(frame.constants.mapped(), &c, sizeof(FrameConstants));

    m_previousViewProjection = viewProjection;
}

VkDeviceAddress GpuScene::frameConstantsAddress() const {
    return m_slots[m_slot].constants.address();
}

} // namespace ghost::graphics::scene
