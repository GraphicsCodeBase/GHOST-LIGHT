// Texture viewer: draws any render graph texture over the final image (inset or full screen), with channel selection
// and a value range, so every G-buffer target and technique output can be inspected live (UI/TextureViewerPanel).
#pragma once

#include "Graphics/ShaderCompiler/PipelineLibrary.h"

#include <string>

#include <volk.h>

namespace ghost::graphics::rendergraph {
class RenderGraph;
}

namespace ghost::graphics::passes {

class TextureViewerPass {
public:
    // OctahedralXy / Zw decode a normal packed with ShaderLibrary/Packing (gbuffer.normal: xy shading, zw geometric).
    enum class Channels : uint32_t { Rgb = 0, Red = 1, Green = 2, Blue = 3, Alpha = 4, SignedRgb = 5, OctahedralXy = 6, OctahedralZw = 7 };

    struct Settings {
        std::string texture;          // render graph name, empty = off
        Channels channels = Channels::Rgb;
        float rangeMin = 0.0f;        // mapped to black
        float rangeMax = 1.0f;        // mapped to white
        bool fullscreen = false;      // else an inset in the bottom-right corner
        float insetScale = 0.4f;      // inset width as a fraction of the screen
    };

    void initialize(shader::PipelineLibrary& pipelines, VkFormat outputFormat);
    // Draws settings.texture onto `output` (Load). Adds nothing when the texture is off, not produced by this graph,
    // an integer format or the output itself.
    void addTo(rendergraph::RenderGraph& graph, const std::string& output);

    static bool isViewable(VkFormat format);

    Settings settings;

private:
    shader::PipelineLibrary* m_pipelines = nullptr;
    shader::PipelineHandle m_pipeline = shader::kInvalidPipeline;
};

} // namespace ghost::graphics::passes
