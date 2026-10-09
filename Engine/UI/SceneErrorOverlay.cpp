// Scene problem panel: bottom of the screen, errors in red, warnings in amber.
#include "UI/SceneErrorOverlay.h"

#include <imgui.h>

namespace ghost::ui {

void SceneErrorOverlay::draw(std::vector<std::string>& errors, std::vector<std::string>& warnings) {
    if (errors.empty() && warnings.empty()) {
        return;
    }
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f, viewport->WorkPos.y + viewport->WorkSize.y - 12.0f),
                            ImGuiCond_Always, ImVec2(0.5f, 1.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(viewport->WorkSize.x * 0.9f, viewport->WorkSize.y * 0.4f));
    ImGui::SetNextWindowBgAlpha(0.92f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove;
    if (ImGui::Begin(errors.empty() ? "Scene warnings" : "Scene problems", nullptr, flags)) {
        if (!errors.empty()) {
            ImGui::TextUnformatted("The rest of the scene loaded. Fix the file and save: it reloads automatically.");
            ImGui::Separator();
        }
        for (const std::string& error : errors) {
            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "%s", error.c_str());
        }
        for (const std::string& warning : warnings) {
            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "%s", warning.c_str());
        }
        if (ImGui::Button("Dismiss")) {
            errors.clear();
            warnings.clear();
        }
    }
    ImGui::End();
}

} // namespace ghost::ui
