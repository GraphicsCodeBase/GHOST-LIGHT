// G-buffer pass: rasterizes the GPU scene into the gbuffer.* textures (layout in ShaderLibrary/GBuffer.slang).
#pragma once

#include "Graphics/ShaderCompiler/PipelineLibrary.h"

#include <array>

namespace ghost::graphics::rendergraph {
class RenderGraph;
}
namespace ghost::graphics::scene {
class GpuScene;
}

namespace ghost::graphics::passes {

class GBufferPass {
public:
    // G-buffer texture names (other passes and techniques read them by name).
    static constexpr const char* kDepth = "gbuffer.depth";
    static constexpr const char* kNormal = "gbuffer.normal";
    static constexpr const char* kAlbedo = "gbuffer.albedo";
    static constexpr const char* kMaterial = "gbuffer.material";
    static constexpr const char* kEmissive = "gbuffer.emissive";
    static constexpr const char* kMotion = "gbuffer.motion";
    static constexpr const char* kEntityId = "gbuffer.entityId";

    void initialize(shader::PipelineLibrary& pipelines);
    void addTo(rendergraph::RenderGraph& graph, const scene::GpuScene& scene);

private:
    shader::PipelineLibrary* m_pipelines = nullptr;
    // One pipeline per GpuScene::DrawCategory: opaque (back-face culled), double-sided, alpha-masked.
    std::array<shader::PipelineHandle, 3> m_pipelineHandles{shader::kInvalidPipeline, shader::kInvalidPipeline, shader::kInvalidPipeline};
};

} // namespace ghost::graphics::passes
