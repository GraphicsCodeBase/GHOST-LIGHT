// Tonemap: samples scene.color, applies exposure + a filmic curve, and writes display-encoded pixels to the swapchain.
#pragma once

#include "Graphics/ShaderCompiler/PipelineLibrary.h"

#include <string>

#include <volk.h>

namespace ghost::graphics::rendergraph {
class RenderGraph;
}

namespace ghost::graphics::passes {

class TonemapPass {
public:
    void initialize(shader::PipelineLibrary& pipelines, VkFormat outputFormat);
    // Reads "scene.color", writes `output` (an external texture, normally "swapchain").
    void addTo(rendergraph::RenderGraph& graph, const std::string& output);

    float exposureEv = 0.0f; // exposure in stops; 0 = no change

private:
    shader::PipelineLibrary* m_pipelines = nullptr;
    shader::PipelineHandle m_pipeline = shader::kInvalidPipeline;
};

} // namespace ghost::graphics::passes
