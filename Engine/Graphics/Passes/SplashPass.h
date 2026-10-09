// Placeholder full-screen pass (until the render graph exists): draws the ghost-light glow into the swapchain image.
#pragma once

#include "Graphics/ShaderCompiler/PipelineLibrary.h"

#include <volk.h>

namespace ghost::graphics::passes {

class SplashPass {
public:
    void initialize(shader::PipelineLibrary& pipelines, VkFormat colorFormat);
    // Records into an already-begun dynamic rendering scope. Draws nothing while the shader has never compiled.
    void record(VkCommandBuffer cmd, VkExtent2D extent, float timeSeconds) const;

private:
    shader::PipelineLibrary* m_pipelines = nullptr;
    shader::PipelineHandle m_pipeline = shader::kInvalidPipeline;
};

} // namespace ghost::graphics::passes
