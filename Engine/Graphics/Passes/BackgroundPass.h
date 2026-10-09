// Procedural HDR sky into scene.color (compute). Placeholder scene until the G-buffer and lighting passes exist.
#pragma once

#include "Graphics/ShaderCompiler/PipelineLibrary.h"

namespace ghost::graphics::rendergraph {
class RenderGraph;
}

namespace ghost::graphics::passes {

class BackgroundPass {
public:
    void initialize(shader::PipelineLibrary& pipelines);
    // Adds the pass; it creates "scene.color" (RGBA16F, full resolution).
    void addTo(rendergraph::RenderGraph& graph);
    void setTime(float timeSeconds) { m_time = timeSeconds; }

private:
    shader::PipelineLibrary* m_pipelines = nullptr;
    shader::PipelineHandle m_pipeline = shader::kInvalidPipeline;
    float m_time = 0.0f;
};

} // namespace ghost::graphics::passes
