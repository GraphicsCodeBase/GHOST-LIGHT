// TechniqueContext: name -> render graph handle lookups, pipeline binding with pass-type checks, one-time error reports.
#include "Graphics/TechniqueRuntime/TechniqueContext.h"

#include "Core/Log.h"
#include "Graphics/GpuScene/GpuScene.h"
#include "Graphics/TechniqueRuntime/TechniqueInstance.h"

#include <format>

namespace ghost::graphics::techniques {

TechniqueContext::TechniqueContext(rendergraph::PassContext& pass, TechniqueInstance& instance, shader::PipelineLibrary& pipelines,
                                   uint32_t passIndex, rendergraph::PassKind kind, const Resources& resources, const scene::GpuScene& scene)
    : m_pass(pass), m_instance(instance), m_pipelines(pipelines), m_passIndex(passIndex), m_kind(kind), m_resources(resources), m_scene(scene),
      m_sceneChanged(instance.sceneChanged), m_lightsChanged(instance.lightsChanged) {}

void TechniqueContext::reportOnce(const std::string& message) const {
    if (m_instance.reported.insert(message).second) {
        core::Log::error("Technique {}: {}", m_instance.technique->id(), message);
    }
}

const rendergraph::TextureHandle* TechniqueContext::find(const std::unordered_map<std::string, rendergraph::TextureHandle>& map,
                                                          const std::string& name, const char* declaredWith) const {
    const auto found = map.find(name);
    if (found == map.end()) {
        reportOnce(std::format("texture '{}' is used in execute() but this pass did not declare it with {} in setup()", name, declaredWith));
        return nullptr;
    }
    return &found->second;
}

uint32_t TechniqueContext::sampled(const std::string& texture) const {
    const rendergraph::TextureHandle* handle = find(m_resources.sampled, texture, "read()");
    return handle ? m_pass.sampledIndex(*handle) : 0;
}

uint32_t TechniqueContext::storage(const std::string& texture) const {
    const rendergraph::TextureHandle* handle = find(m_resources.storage, texture, "write(), readWrite() or history()");
    return handle ? m_pass.storageIndex(*handle) : 0;
}

uint32_t TechniqueContext::previous(const std::string& texture) const {
    const rendergraph::TextureHandle* handle = find(m_resources.previous, texture, "history()");
    return handle ? m_pass.sampledIndex(*handle) : 0;
}

bool TechniqueContext::historyValid(const std::string& texture) const {
    const rendergraph::TextureHandle* handle = find(m_resources.previous, texture, "history()");
    return handle && m_pass.historyValid(*handle);
}

VkExtent2D TechniqueContext::extent(const std::string& texture) const {
    const auto storageHandle = m_resources.storage.find(texture);
    if (storageHandle != m_resources.storage.end()) {
        return m_pass.extent(storageHandle->second);
    }
    const rendergraph::TextureHandle* handle = find(m_resources.sampled, texture, "read() or write()");
    return handle ? m_pass.extent(*handle) : VkExtent2D{};
}

VkExtent2D TechniqueContext::renderSize() const {
    const glm::vec2 size = m_scene.frameConstants().renderSize;
    return {static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y)};
}

VkDeviceAddress TechniqueContext::buffer(const std::string& name) const {
    const auto found = m_resources.buffers.find(name);
    if (found == m_resources.buffers.end()) {
        reportOnce(std::format("buffer '{}' is used in execute() but this pass did not declare it with buffer() in setup()", name));
        return 0;
    }
    return m_pass.bufferAddress(found->second);
}

VkDeviceAddress TechniqueContext::frameConstants() const {
    return m_scene.frameConstantsAddress();
}

void TechniqueContext::dispatch(const std::string& pipeline, VkExtent2D size, uint32_t groupX, uint32_t groupY) const {
    const auto found = m_instance.computePipelines.find(pipeline);
    if (found == m_instance.computePipelines.end()) {
        reportOnce(std::format("compute pipeline '{}' was not declared with computePipeline() in setup()", pipeline));
        return;
    }
    if (m_kind != rendergraph::PassKind::Compute) {
        reportOnce(std::format("compute pipeline '{}' used in a ray tracing pass; declare the pass with PassType::Compute", pipeline));
        return;
    }
    const VkPipeline handle = m_pipelines.pipeline(found->second);
    if (handle == VK_NULL_HANDLE) {
        return; // shader error: shown in the overlay, the frame continues without this pass
    }
    m_pass.bindPipeline(handle);
    m_pass.dispatchForSize(size, groupX, groupY);
}

void TechniqueContext::traceRays(const std::string& pipeline, VkExtent2D size) const {
    const auto found = m_instance.rayPipelines.find(pipeline);
    if (found == m_instance.rayPipelines.end()) {
        reportOnce(std::format("ray pipeline '{}' was not declared with rayPipeline() in setup()", pipeline));
        return;
    }
    if (m_kind != rendergraph::PassKind::RayTracing) {
        reportOnce(std::format("ray pipeline '{}' used in a compute pass; declare the pass with PassType::RayTracing", pipeline));
        return;
    }
    raytracing::RayTracingPipeline& rayPipeline = *found->second;
    if (!rayPipeline.prepare(m_pass.frameIndex() - 1)) {
        return;
    }
    m_pass.bindPipeline(rayPipeline.pipeline());
    rayPipeline.traceRays(m_pass.cmd(), size.width, size.height);
}

} // namespace ghost::graphics::techniques
