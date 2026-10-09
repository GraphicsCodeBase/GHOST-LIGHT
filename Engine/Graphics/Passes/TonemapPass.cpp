// TonemapPass: full-screen raster pass from scene.color to the output attachment.
#include "Graphics/Passes/TonemapPass.h"

#include "Core/Paths.h"
#include "Graphics/RenderGraph/RenderGraph.h"

#include <memory>

namespace ghost::graphics::passes {

namespace {

struct TonemapConstants {
    uint32_t input;
    float exposure;
};

} // namespace

void TonemapPass::initialize(shader::PipelineLibrary& pipelines, VkFormat outputFormat) {
    m_pipelines = &pipelines;
    const std::filesystem::path shaders = core::Paths::root() / "Engine" / "Graphics" / "Passes" / "Shaders";
    shader::GraphicsPipelineDesc desc;
    desc.name = "Tonemap";
    desc.vertex = {shaders / "Fullscreen.slang", "vertexMain", shader::ShaderStage::Vertex};
    desc.fragment = {shaders / "Tonemap.slang", "fragmentMain", shader::ShaderStage::Fragment};
    desc.colorFormats = {outputFormat};
    m_pipeline = pipelines.addGraphics(desc);
}

void TonemapPass::addTo(rendergraph::RenderGraph& graph, const std::string& output) {
    auto input = std::make_shared<rendergraph::TextureHandle>();
    graph.addPass(
        "Tonemap", rendergraph::PassKind::Raster,
        [input, output](rendergraph::PassBuilder& builder) {
            *input = builder.sample("scene.color");
            builder.colorAttachment(output, rendergraph::LoadOp::DontCare);
        },
        [this, input](rendergraph::PassContext& ctx) {
            const VkPipeline pipeline = m_pipelines->pipeline(m_pipeline);
            if (pipeline == VK_NULL_HANDLE) {
                return;
            }
            ctx.bindPipeline(pipeline);
            ctx.pushConstants(TonemapConstants{ctx.sampledIndex(*input), exposure});
            ctx.drawFullscreenTriangle();
        });
}

} // namespace ghost::graphics::passes
