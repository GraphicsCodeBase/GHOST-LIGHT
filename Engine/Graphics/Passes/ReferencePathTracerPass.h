// Reference path tracer pass: a naive progressive path tracer (ray tracing pipeline) that accumulates one sample per
// pixel per frame into pathtracer.accumulation and writes the running mean to scene.color. Restarts whenever the
// camera, the scene or the resolution changes.
#pragma once

#include "Graphics/RayTracing/RayTracingPipeline.h"

#include <cstdint>

#include <glm/mat4x4.hpp>
#include <volk.h>

namespace ghost::graphics::vulkan {
class Device;
class DeletionQueue;
} // namespace ghost::graphics::vulkan
namespace ghost::graphics::shader {
class PipelineLibrary;
}
namespace ghost::graphics::rendergraph {
class RenderGraph;
}
namespace ghost::graphics::scene {
class GpuScene;
}

namespace ghost::graphics::passes {

class ReferencePathTracerPass {
public:
    static constexpr const char* kAccumulation = "pathtracer.accumulation";

    void initialize(shader::PipelineLibrary& pipelines, const vulkan::Device& device, vulkan::DeletionQueue& deletionQueue);
    void shutdown();
    void addTo(rendergraph::RenderGraph& graph, const scene::GpuScene& scene);

    void resetAccumulation() { m_resetRequested = true; }
    uint32_t sampleCount() const { return m_sampleCount; }

    uint32_t maxBounces = 4;
    uint32_t maxSamples = 0; // stop accumulating after this many samples; 0 = never stop

private:
    bool needsRestart(const scene::GpuScene& scene, VkExtent2D extent) const;

    raytracing::RayTracingPipeline m_pipeline;
    uint32_t m_sampleCount = 0;
    bool m_resetRequested = true;
    glm::mat4 m_lastViewProjection{0.0f};
    uint64_t m_lastSceneRevision = UINT64_MAX;
    VkExtent2D m_lastExtent{};
    uint32_t m_lastMaxBounces = 0;
};

} // namespace ghost::graphics::passes
