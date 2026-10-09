// ReferencePathTracerPass: restart detection on the CPU, one TraceRays per frame over the render resolution.
#include "Graphics/Passes/ReferencePathTracerPass.h"

#include "Core/Paths.h"
#include "Graphics/GpuScene/GpuScene.h"
#include "Graphics/RenderGraph/RenderGraph.h"

#include <memory>

namespace ghost::graphics::passes {

namespace {

// Mirror of PathTracerConstants in ReferencePathTracer.slang.
struct PathTracerConstants {
    VkDeviceAddress frame;
    uint32_t accumulation;
    uint32_t output;
    uint32_t sampleIndex;
    uint32_t maxBounces;
    uint32_t traceSample; // 0 = converged: only copy the mean to the output
    uint32_t padding;
};

struct Handles {
    rendergraph::TextureHandle accumulation;
    rendergraph::TextureHandle output;
};

} // namespace

void ReferencePathTracerPass::initialize(shader::PipelineLibrary& pipelines, const vulkan::Device& device, vulkan::DeletionQueue& deletionQueue) {
    const std::filesystem::path file = core::Paths::root() / "Engine" / "Graphics" / "Passes" / "Shaders" / "ReferencePathTracer.slang";
    raytracing::RayTracingPipelineDesc desc;
    desc.name = "ReferencePathTracer";
    desc.rayGeneration = {file, "rayGeneration", shader::ShaderStage::RayGeneration};
    desc.misses = {{file, "miss", shader::ShaderStage::Miss}};
    desc.hitGroups = {{shader::ShaderEntryPoint{file, "closestHit", shader::ShaderStage::ClosestHit},
                       shader::ShaderEntryPoint{file, "anyHitAlphaTest", shader::ShaderStage::AnyHit}}};
    m_pipeline.initialize(pipelines, device, deletionQueue, std::move(desc));
}

void ReferencePathTracerPass::shutdown() {
    m_pipeline.shutdown();
}

bool ReferencePathTracerPass::needsRestart(const scene::GpuScene& scene, VkExtent2D extent) const {
    return m_resetRequested || scene.frameConstants().viewProjection != m_lastViewProjection || scene.sceneRevision() != m_lastSceneRevision ||
           extent.width != m_lastExtent.width || extent.height != m_lastExtent.height || maxBounces != m_lastMaxBounces;
}

void ReferencePathTracerPass::addTo(rendergraph::RenderGraph& graph, const scene::GpuScene& scene) {
    auto handles = std::make_shared<Handles>();
    graph.addPass(
        "PathTracer", rendergraph::PassKind::RayTracing,
        [handles](rendergraph::PassBuilder& builder) {
            builder.create(kAccumulation, {VK_FORMAT_R32G32B32A32_SFLOAT, rendergraph::Scale::Full, rendergraph::Lifetime::Persistent});
            handles->accumulation = builder.readWriteStorage(kAccumulation);
            handles->output = builder.writeStorage("scene.color", {VK_FORMAT_R16G16B16A16_SFLOAT, rendergraph::Scale::Full});
        },
        [this, handles, &scene](rendergraph::PassContext& ctx) {
            if (!m_pipeline.prepare(ctx.frameIndex() - 1)) {
                return;
            }
            const VkExtent2D extent = ctx.extent(handles->output);
            if (needsRestart(scene, extent)) {
                m_sampleCount = 0;
                m_resetRequested = false;
                m_lastViewProjection = scene.frameConstants().viewProjection;
                m_lastSceneRevision = scene.sceneRevision();
                m_lastExtent = extent;
                m_lastMaxBounces = maxBounces;
            }
            // Converged: keep showing the mean without tracing more.
            const bool tracing = maxSamples == 0 || m_sampleCount < maxSamples;
            ctx.bindPipeline(m_pipeline.pipeline());
            ctx.pushConstants(PathTracerConstants{scene.frameConstantsAddress(), ctx.storageIndex(handles->accumulation),
                                                  ctx.storageIndex(handles->output), m_sampleCount, maxBounces, tracing ? 1u : 0u, 0u});
            m_pipeline.traceRays(ctx.cmd(), extent.width, extent.height);
            if (tracing) {
                ++m_sampleCount;
            }
        });
}

} // namespace ghost::graphics::passes
