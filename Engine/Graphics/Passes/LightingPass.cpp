// LightingPass: one compute dispatch over scene.color, reading the G-buffer through bindless indices.
#include "Graphics/Passes/LightingPass.h"

#include "Core/Paths.h"
#include "Graphics/GpuScene/GpuScene.h"
#include "Graphics/Passes/GBufferPass.h"
#include "Graphics/RenderGraph/RenderGraph.h"

#include <memory>

namespace ghost::graphics::passes {

namespace {

// Mirror of LightingConstants in DeferredLighting.slang (GBufferIndices order: depth, normal, albedo, material, emissive).
struct LightingConstants {
    VkDeviceAddress frame;
    uint32_t depth;
    uint32_t normal;
    uint32_t albedo;
    uint32_t material;
    uint32_t emissive;
    uint32_t output;
};
static_assert(sizeof(LightingConstants) == 32);

struct Handles {
    rendergraph::TextureHandle depth, normal, albedo, material, emissive, output;
};

} // namespace

void LightingPass::initialize(shader::PipelineLibrary& pipelines) {
    m_pipelines = &pipelines;
    const std::filesystem::path file = core::Paths::root() / "Engine" / "Graphics" / "Passes" / "Shaders" / "DeferredLighting.slang";
    m_pipeline = pipelines.addCompute({"DeferredLighting", {file, "main", shader::ShaderStage::Compute}});
}

void LightingPass::addTo(rendergraph::RenderGraph& graph, const scene::GpuScene& scene) {
    auto handles = std::make_shared<Handles>();
    graph.addPass(
        "Lighting", rendergraph::PassKind::Compute,
        [handles](rendergraph::PassBuilder& builder) {
            handles->depth = builder.sample(GBufferPass::kDepth);
            handles->normal = builder.sample(GBufferPass::kNormal);
            handles->albedo = builder.sample(GBufferPass::kAlbedo);
            handles->material = builder.sample(GBufferPass::kMaterial);
            handles->emissive = builder.sample(GBufferPass::kEmissive);
            handles->output = builder.writeStorage("scene.color", {VK_FORMAT_R16G16B16A16_SFLOAT, rendergraph::Scale::Full});
        },
        [this, handles, &scene](rendergraph::PassContext& ctx) {
            const VkPipeline pipeline = m_pipelines->pipeline(m_pipeline);
            if (pipeline == VK_NULL_HANDLE) {
                return;
            }
            const Handles& h = *handles;
            ctx.bindPipeline(pipeline);
            ctx.pushConstants(LightingConstants{scene.frameConstantsAddress(), ctx.sampledIndex(h.depth), ctx.sampledIndex(h.normal),
                                                ctx.sampledIndex(h.albedo), ctx.sampledIndex(h.material), ctx.sampledIndex(h.emissive),
                                                ctx.storageIndex(h.output)});
            ctx.dispatchForSize(ctx.extent(h.output));
        });
}

} // namespace ghost::graphics::passes
