// GBufferPass: three pipelines (opaque, double-sided, alpha-masked), one indexed draw per scene draw.
#include "Graphics/Passes/GBufferPass.h"

#include "Core/Paths.h"
#include "Graphics/GpuScene/GpuScene.h"
#include "Graphics/RenderGraph/RenderGraph.h"

namespace ghost::graphics::passes {

namespace {

struct GBufferConstants {
    VkDeviceAddress frame;
};

constexpr VkFormat kDepthFormat = VK_FORMAT_D32_SFLOAT;

} // namespace

void GBufferPass::initialize(shader::PipelineLibrary& pipelines) {
    m_pipelines = &pipelines;
    const std::filesystem::path file = core::Paths::root() / "Engine" / "Graphics" / "Passes" / "Shaders" / "GBufferFill.slang";
    struct Variant {
        const char* name;
        const char* fragment;
        VkCullModeFlags cull;
    };
    const Variant variants[] = {
        {"GBuffer.Opaque", "fragmentOpaque", VK_CULL_MODE_BACK_BIT},
        {"GBuffer.DoubleSided", "fragmentOpaque", VK_CULL_MODE_NONE},
        {"GBuffer.AlphaMasked", "fragmentMasked", VK_CULL_MODE_NONE},
    };
    for (size_t i = 0; i < std::size(variants); ++i) {
        shader::GraphicsPipelineDesc desc;
        desc.name = variants[i].name;
        desc.vertex = {file, "vertexMain", shader::ShaderStage::Vertex};
        desc.fragment = {file, variants[i].fragment, shader::ShaderStage::Fragment};
        desc.colorFormats = {VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8_UNORM,
                             VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R32_UINT};
        desc.depthFormat = kDepthFormat;
        desc.depthTest = true;
        desc.depthWrite = true;
        desc.cullMode = variants[i].cull;
        // glTF front faces are counter-clockwise; the projection's Y flip keeps the image upright, so winding is preserved.
        desc.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        m_pipelineHandles[i] = pipelines.addGraphics(desc);
    }
}

void GBufferPass::addTo(rendergraph::RenderGraph& graph, const scene::GpuScene& scene) {
    using rendergraph::TextureDesc;
    graph.addPass(
        "GBuffer", rendergraph::PassKind::Raster,
        [](rendergraph::PassBuilder& builder) {
            builder.create(kNormal, TextureDesc{VK_FORMAT_R16G16B16A16_SNORM});
            builder.create(kAlbedo, TextureDesc{VK_FORMAT_R8G8B8A8_SRGB});
            builder.create(kMaterial, TextureDesc{VK_FORMAT_R8G8_UNORM});
            builder.create(kEmissive, TextureDesc{VK_FORMAT_R16G16B16A16_SFLOAT});
            builder.create(kMotion, TextureDesc{VK_FORMAT_R16G16_SFLOAT});
            builder.create(kEntityId, TextureDesc{VK_FORMAT_R32_UINT});
            builder.create(kDepth, TextureDesc{kDepthFormat});
            // Attachment order = SV_Target order in GBufferFill.slang.
            builder.colorAttachment(kNormal);
            builder.colorAttachment(kAlbedo);
            builder.colorAttachment(kMaterial);
            builder.colorAttachment(kEmissive);
            builder.colorAttachment(kMotion);
            builder.colorAttachment(kEntityId);
            builder.depthAttachment(kDepth);
        },
        [this, &scene](rendergraph::PassContext& ctx) {
            if (!scene.hasGeometry()) {
                return;
            }
            const VkCommandBuffer cmd = ctx.cmd();
            vkCmdBindIndexBuffer(cmd, scene.indexBuffer(), 0, VK_INDEX_TYPE_UINT32);
            for (uint32_t category = 0; category < m_pipelineHandles.size(); ++category) {
                const auto& draws = scene.drawCommands(static_cast<scene::GpuScene::DrawCategory>(category));
                const VkPipeline pipeline = m_pipelines->pipeline(m_pipelineHandles[category]);
                if (draws.empty() || pipeline == VK_NULL_HANDLE) {
                    continue;
                }
                ctx.bindPipeline(pipeline);
                ctx.pushConstants(GBufferConstants{scene.frameConstantsAddress()});
                for (const scene::GpuScene::DrawCommand& draw : draws) {
                    vkCmdDrawIndexed(cmd, draw.indexCount, 1, draw.firstIndex, draw.vertexOffset, draw.drawIndex);
                }
            }
        });
}

} // namespace ghost::graphics::passes
