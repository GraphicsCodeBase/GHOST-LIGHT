// TextureViewerPanel: a texture combo built from the render graph's current textures, channel/range widgets and presets.
#include "UI/TextureViewerPanel.h"

#include "Graphics/Passes/TextureViewerPass.h"
#include "Graphics/RenderGraph/RenderGraph.h"
#include "Graphics/Renderer/Renderer.h"

#include <format>
#include <string>

#include <imgui.h>

namespace ghost::ui {

namespace {

using graphics::passes::TextureViewerPass;

const char* formatName(VkFormat format) {
    switch (format) {
    case VK_FORMAT_R8G8B8A8_UNORM: return "RGBA8";
    case VK_FORMAT_R8G8B8A8_SRGB: return "RGBA8 sRGB";
    case VK_FORMAT_B8G8R8A8_UNORM: return "BGRA8";
    case VK_FORMAT_R8G8_UNORM: return "RG8";
    case VK_FORMAT_R8_UNORM: return "R8";
    case VK_FORMAT_R16_SFLOAT: return "R16F";
    case VK_FORMAT_R16G16_SFLOAT: return "RG16F";
    case VK_FORMAT_R16G16B16A16_SFLOAT: return "RGBA16F";
    case VK_FORMAT_R16G16B16A16_SNORM: return "RGBA16 SNORM";
    case VK_FORMAT_R32_SFLOAT: return "R32F";
    case VK_FORMAT_R32G32_SFLOAT: return "RG32F";
    case VK_FORMAT_R32G32B32A32_SFLOAT: return "RGBA32F";
    case VK_FORMAT_R32_UINT: return "R32 UINT";
    case VK_FORMAT_D32_SFLOAT: return "D32F";
    default: return "other";
    }
}

bool contains(const std::string& text, const char* part) {
    return text.find(part) != std::string::npos;
}

// A sensible starting view for the texture's name; the user can change everything afterwards.
void applyPreset(TextureViewerPass::Settings& s) {
    const std::string& n = s.texture;
    if (contains(n, "depth")) {
        s.channels = TextureViewerPass::Channels::Red; // reverse-Z: near / distance, so values are small
        s.rangeMin = 0.0f;
        s.rangeMax = 0.05f;
    } else if (n == "gbuffer.normal") {
        s.channels = TextureViewerPass::Channels::OctahedralXy; // ShaderLibrary/GBuffer.slang: octahedral, xy shading / zw geometric
    } else if (contains(n, "normal")) {
        s.channels = TextureViewerPass::Channels::SignedRgb;
        s.rangeMin = -1.0f;
        s.rangeMax = 1.0f;
    } else if (contains(n, "motion")) {
        s.channels = TextureViewerPass::Channels::SignedRgb; // uv offsets per frame: tiny
        s.rangeMin = -0.01f;
        s.rangeMax = 0.01f;
    } else if (contains(n, "color") || contains(n, "accumulation") || contains(n, "emissive")) {
        s.channels = TextureViewerPass::Channels::Rgb; // radiance in nits
        s.rangeMin = 0.0f;
        s.rangeMax = 1000.0f;
    } else {
        s.channels = TextureViewerPass::Channels::Rgb;
        s.rangeMin = 0.0f;
        s.rangeMax = 1.0f;
    }
}

} // namespace

void TextureViewerPanel::draw(graphics::Renderer& renderer, bool* open) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 10.0f, viewport->WorkPos.y + viewport->WorkSize.y * 0.45f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Texture viewer (F3)", open)) {
        ImGui::End();
        return;
    }
    TextureViewerPass::Settings& s = renderer.textureViewer().settings;
    const auto textures = renderer.renderGraph().textures();

    const std::string preview = s.texture.empty() ? "(off)" : s.texture;
    if (ImGui::BeginCombo("Texture", preview.c_str())) {
        if (ImGui::Selectable("(off)", s.texture.empty())) {
            s.texture.clear();
        }
        for (const auto& info : textures) {
            if (info.external) {
                continue; // the swapchain is what you are looking at
            }
            const bool viewable = TextureViewerPass::isViewable(info.format);
            const std::string label = std::format("{}  ({}, {}x{}){}", info.name, formatName(info.format), info.extent.width,
                                                  info.extent.height, viewable ? "" : "  integer: not viewable yet");
            ImGui::BeginDisabled(!viewable);
            if (ImGui::Selectable(label.c_str(), info.name == s.texture) && info.name != s.texture) {
                s.texture = info.name;
                applyPreset(s);
            }
            ImGui::EndDisabled();
        }
        ImGui::EndCombo();
    }
    if (s.texture.empty()) {
        ImGui::TextWrapped("Shows any render graph texture: G-buffer targets, scene.color and every technique output.");
        ImGui::End();
        return;
    }

    const char* channelNames[] = {"RGB", "R", "G", "B", "A", "Signed RGB (x*0.5+0.5)", "Octahedral normal (xy)", "Octahedral normal (zw)"};
    int channels = static_cast<int>(s.channels);
    if (ImGui::Combo("Channels", &channels, channelNames, IM_ARRAYSIZE(channelNames))) {
        s.channels = static_cast<TextureViewerPass::Channels>(channels);
    }
    if (s.channels == TextureViewerPass::Channels::SignedRgb) {
        ImGui::DragFloat("Max |value|", &s.rangeMax, s.rangeMax * 0.01f + 1e-5f, 1e-5f, 1e6f, "%.5g", ImGuiSliderFlags_Logarithmic);
    } else if (s.channels != TextureViewerPass::Channels::OctahedralXy && s.channels != TextureViewerPass::Channels::OctahedralZw) {
        ImGui::DragFloat("Black at", &s.rangeMin, (s.rangeMax - s.rangeMin) * 0.005f + 1e-5f, -1e6f, 1e6f, "%.5g");
        ImGui::DragFloat("White at", &s.rangeMax, (s.rangeMax - s.rangeMin) * 0.005f + 1e-5f, -1e6f, 1e6f, "%.5g");
    }
    if (ImGui::Button("Preset for this texture")) {
        applyPreset(s);
    }
    ImGui::SameLine();
    if (ImGui::Button("0..1")) {
        s.rangeMin = 0.0f;
        s.rangeMax = 1.0f;
    }
    ImGui::Checkbox("Full screen", &s.fullscreen);
    if (!s.fullscreen) {
        ImGui::SameLine();
        ImGui::SliderFloat("Size", &s.insetScale, 0.1f, 1.0f, "%.2f");
    }
    ImGui::TextDisabled("Point sampled. NaN / inf show as magenta.");
    ImGui::End();
}

} // namespace ghost::ui
