// The scene's ray tracing acceleration structures: one compacted BLAS per mesh (rebuilt when the GPU scene's geometry
// changes) and one TLAS per frame in flight (rebuilt every frame from the GPU scene's instances). Shaders reach this
// frame's TLAS through the bindless set: gSceneTlas[frame->tlasIndex] (ShaderLibrary/Scene.slang: sceneTlas(frame)).
#pragma once

#include "Graphics/RayTracing/AccelerationStructure.h"
#include "Graphics/Vulkan/GpuBuffer.h"
#include "Graphics/Vulkan/ImmediateSubmit.h"

#include <array>
#include <cstdint>
#include <vector>

#include <volk.h>

namespace ghost::graphics::vulkan {
class Device;
class BindlessDescriptors;
class DeletionQueue;
} // namespace ghost::graphics::vulkan
namespace ghost::graphics::scene {
class GpuScene;
}

namespace ghost::graphics::raytracing {

class SceneAccelerationStructures {
public:
    static constexpr uint32_t kFrameSlots = 2;

    SceneAccelerationStructures() = default;
    ~SceneAccelerationStructures();
    SceneAccelerationStructures(const SceneAccelerationStructures&) = delete;
    SceneAccelerationStructures& operator=(const SceneAccelerationStructures&) = delete;

    void initialize(const vulkan::Device& device, vulkan::BindlessDescriptors& bindless, vulkan::DeletionQueue& deletionQueue);
    void shutdown();

    // Between frames (blocking, load time): rebuilds every BLAS if the scene's geometry changed since the last call.
    void updateBlases(const scene::GpuScene& scene, uint64_t lastSubmittedFrame);
    // While recording frame slot `slot`: rebuilds that slot's TLAS from the scene's instances, then makes it visible to
    // every later command of the frame. Instance custom index = index into the frame's GpuInstance buffer.
    void recordTlasBuild(VkCommandBuffer cmd, uint32_t slot, const scene::GpuScene& scene);

    size_t blasCount() const { return m_blases.size(); }
    VkDeviceSize blasBytes() const;
    uint32_t tlasInstanceCount() const { return m_lastInstanceCount; }

private:
    struct TlasSlot {
        AccelerationStructure tlas;
        vulkan::GpuBuffer instances; // VkAccelerationStructureInstanceKHR[capacity], written by the CPU each frame
        vulkan::GpuBuffer scratch;
        uint32_t capacity = 0;
    };

    void ensureTlasCapacity(uint32_t slot, uint32_t instanceCount);
    VkDeviceAddress alignedScratchAddress(const vulkan::GpuBuffer& scratch) const;

    const vulkan::Device* m_device = nullptr;
    vulkan::BindlessDescriptors* m_bindless = nullptr;
    vulkan::DeletionQueue* m_deletionQueue = nullptr;
    vulkan::ImmediateSubmit m_submit;
    std::vector<AccelerationStructure> m_blases; // indexed like GpuScene::meshes()
    uint64_t m_geometryRevision = UINT64_MAX;
    std::array<TlasSlot, kFrameSlots> m_slots;
    uint32_t m_lastInstanceCount = 0;
};

} // namespace ghost::graphics::raytracing
