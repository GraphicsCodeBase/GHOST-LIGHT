// Splash pass: fullscreen triangle + procedural glow, with time and aspect ratio in push constants.
#include "Graphics/Passes/SplashPass.h"

#include "Core/Paths.h"

namespace ghost::graphics::passes {

namespace {

struct SplashConstants {
    float time;
    float aspect;
};

} // namespace

void SplashPass::initialize(shader::PipelineLibrary& pipelines, VkFormat colorFormat) {
    m_pipelines = &pipelines;
    const std::filesystem::path shaders = core::Paths::root() / "Engine" / "Graphics" / "Passes" / "Shaders";

    shader::GraphicsPipelineDesc desc;
    desc.name = "Splash";
    desc.vertex = {shaders / "Fullscreen.slang", "vertexMain", shader::ShaderStage::Vertex};
    desc.fragment = {shaders / "Splash.slang", "fragmentMain", shader::ShaderStage::Fragment};
    desc.colorFormats = {colorFormat};
    m_pipeline = pipelines.addGraphics(desc);
}

void SplashPass::record(VkCommandBuffer cmd, VkExtent2D extent, float timeSeconds) const {
    const VkPipeline pipeline = m_pipelines->pipeline(m_pipeline);
    if (pipeline == VK_NULL_HANDLE) {
        return;
    }
    const VkViewport viewport{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};
    const VkRect2D scissor{{0, 0}, extent};
    const SplashConstants constants{timeSeconds, static_cast<float>(extent.width) / static_cast<float>(extent.height)};

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    vkCmdPushConstants(cmd, m_pipelines->layout(), VK_SHADER_STAGE_ALL, 0, sizeof(constants), &constants);
    vkCmdDraw(cmd, 3, 1, 0, 0);
}

} // namespace ghost::graphics::passes
