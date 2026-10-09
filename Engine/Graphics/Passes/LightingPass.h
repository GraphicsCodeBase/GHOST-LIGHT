// Lighting pass: placeholder deferred lighting from the G-buffer into scene.color (sun + point/spot lights, no shadows).
#pragma once

#include "Graphics/ShaderCompiler/PipelineLibrary.h"

namespace ghost::graphics::rendergraph {
class RenderGraph;
}
namespace ghost::graphics::scene {
class GpuScene;
}

namespace ghost::graphics::passes {

class LightingPass {
public:
    void initialize(shader::PipelineLibrary& pipelines);
    // Reads the gbuffer.* textures, writes "scene.color" (RGBA16F, linear radiance in nits).
    void addTo(rendergraph::RenderGraph& graph, const scene::GpuScene& scene);

private:
    shader::PipelineLibrary* m_pipelines = nullptr;
    shader::PipelineHandle m_pipeline = shader::kInvalidPipeline;
};

} // namespace ghost::graphics::passes
