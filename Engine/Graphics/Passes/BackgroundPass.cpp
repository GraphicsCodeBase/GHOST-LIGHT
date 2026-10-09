// BackgroundPass: one compute dispatch over scene.color.
#include "Graphics/Passes/BackgroundPass.h"

#include "Core/Paths.h"
#include "Graphics/RenderGraph/RenderGraph.h"

#include <memory>

namespace ghost::graphics::passes {

namespace {

struct BackgroundConstants {
    uint32_t output;
    float time;
    uint32_t width;
    uint32_t height;
};

} // namespace

void BackgroundPass::initialize(shader::PipelineLibrary& pipelines) {
    m_pipelines = &pipelines;
    const std::filesystem::path file = core::Paths::root() / "Engine" / "Graphics" / "Passes" / "Shaders" / "Background.slang";
    m_pipeline = pipelines.addCompute({"Background", {file, "main", shader::ShaderStage::Compute}});
}

void BackgroundPass::addTo(rendergraph::RenderGraph& graph) {
    auto output = std::make_shared<rendergraph::TextureHandle>();
    graph.addPass(
        "Background", rendergraph::PassKind::Compute,
        [output](rendergraph::PassBuilder& builder) {
            *output = builder.writeStorage("scene.color", {VK_FORMAT_R16G16B16A16_SFLOAT, rendergraph::Scale::Full});
        },
        [this, output](rendergraph::PassContext& ctx) {
            const VkPipeline pipeline = m_pipelines->pipeline(m_pipeline);
            if (pipeline == VK_NULL_HANDLE) {
                return;
            }
            const VkExtent2D size = ctx.extent(*output);
            ctx.bindPipeline(pipeline);
            ctx.pushConstants(BackgroundConstants{ctx.storageIndex(*output), m_time, size.width, size.height});
            ctx.dispatchForSize(size);
        });
}

} // namespace ghost::graphics::passes
