// TextureViewerPass: an optional raster pass after Tonemap that samples the chosen texture into a screen rectangle.
#include "Graphics/Passes/TextureViewerPass.h"

#include "Core/Paths.h"
#include "Graphics/RenderGraph/RenderGraph.h"

#include <algorithm>
#include <memory>

namespace ghost::graphics::passes {

namespace {

struct TextureViewerConstants {
    uint32_t input;
    uint32_t channels;
    float rangeMin;
    float rangeMax;
    float rect[4]; // uv min x, min y, max x, max y of the screen rectangle
};

struct ViewState {
    rendergraph::TextureHandle input;
    rendergraph::TextureHandle output;
    bool active = false;
};

} // namespace

bool TextureViewerPass::isViewable(VkFormat format) {
    switch (format) {
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32G32B32A32_SINT:
        return false; // the bindless array is Texture2D<float4>; integer views would need their own array
    default:
        return format != VK_FORMAT_UNDEFINED;
    }
}

void TextureViewerPass::initialize(shader::PipelineLibrary& pipelines, VkFormat outputFormat) {
    m_pipelines = &pipelines;
    const std::filesystem::path shaders = core::Paths::root() / "Engine" / "Graphics" / "Passes" / "Shaders";
    shader::GraphicsPipelineDesc desc;
    desc.name = "TextureViewer";
    desc.vertex = {shaders / "Fullscreen.slang", "vertexMain", shader::ShaderStage::Vertex};
    desc.fragment = {shaders / "TextureViewer.slang", "fragmentMain", shader::ShaderStage::Fragment};
    desc.colorFormats = {outputFormat};
    m_pipeline = pipelines.addGraphics(desc);
}

void TextureViewerPass::addTo(rendergraph::RenderGraph& graph, const std::string& output) {
    if (settings.texture.empty() || settings.texture == output) {
        return;
    }
    auto state = std::make_shared<ViewState>();
    const std::string name = settings.texture;
    graph.addPass(
        "TextureViewer", rendergraph::PassKind::Raster,
        [state, name, output](rendergraph::PassBuilder& builder) {
            // Setups run in pass order, so this sees exactly what the earlier passes of this graph produce.
            const rendergraph::TextureDesc* desc = builder.find(name);
            state->active = desc && isViewable(desc->format);
            if (state->active) {
                state->input = builder.sample(name);
            }
            state->output = builder.colorAttachment(output, rendergraph::LoadOp::Load);
        },
        [this, state](rendergraph::PassContext& ctx) {
            const VkPipeline pipeline = m_pipelines->pipeline(m_pipeline);
            if (!state->active || pipeline == VK_NULL_HANDLE) {
                return;
            }
            const VkExtent2D screen = ctx.extent(state->output);
            const VkExtent2D source = ctx.extent(state->input);
            TextureViewerConstants c{};
            c.input = ctx.sampledIndex(state->input);
            c.channels = static_cast<uint32_t>(settings.channels);
            c.rangeMin = settings.rangeMin;
            c.rangeMax = settings.rangeMax;
            // Keep the texture's aspect ratio: fit it into the screen (full screen) or into an inset of insetScale width.
            const float screenAspect = static_cast<float>(screen.width) / static_cast<float>(std::max(screen.height, 1u));
            const float sourceAspect = static_cast<float>(source.width) / static_cast<float>(std::max(source.height, 1u));
            float width = settings.fullscreen ? 1.0f : std::clamp(settings.insetScale, 0.1f, 1.0f);
            float height = width * screenAspect / sourceAspect;
            if (height > 1.0f) {
                width /= height;
                height = 1.0f;
            }
            const float margin = settings.fullscreen ? 0.0f : 0.01f;
            const float maxX = settings.fullscreen ? 0.5f + width * 0.5f : 1.0f - margin;
            const float maxY = settings.fullscreen ? 0.5f + height * 0.5f : 1.0f - margin * screenAspect;
            c.rect[0] = maxX - width;
            c.rect[1] = maxY - height;
            c.rect[2] = maxX;
            c.rect[3] = maxY;
            ctx.bindPipeline(pipeline);
            ctx.pushConstants(c);
            ctx.drawFullscreenTriangle();
        });
}

} // namespace ghost::graphics::passes
