// Corner stats overlay (replaced by DebugTools' GPU timer panel for per-pass numbers).
#include "UI/PerformanceOverlay.h"

#include <imgui.h>

namespace ghost::ui {

void PerformanceOverlay::draw(const Stats& stats) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 10.0f, viewport->WorkPos.y + 10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.55f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoDocking;
    if (ImGui::Begin("##Performance", nullptr, flags)) {
        ImGui::Text("GHOST LIGHT  %.0f fps  %.2f ms", stats.fps, stats.fps > 0.0 ? 1000.0 / stats.fps : 0.0);
        ImGui::Text("%s  %dx%d", stats.gpuName.c_str(), stats.width, stats.height);
        if (!stats.mode.empty()) {
            ImGui::Text("%s", stats.mode.c_str());
        }
        if (stats.validation) {
            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "Vulkan validation ON (slower)");
        }
        if (!stats.passTimings.empty()) {
            ImGui::Separator();
            ImGui::Text("GPU %.3f ms", stats.gpuMilliseconds);
            for (const auto& [name, ms] : stats.passTimings) {
                ImGui::Text("  %-12s %7.3f ms", name.c_str(), ms);
            }
        }
    }
    ImGui::End();
}

} // namespace ghost::ui
