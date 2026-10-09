// Shader error panel: pinned to the top of the screen, red, with the full compiler output one click away.
#include "UI/ShaderErrorOverlay.h"

#include <imgui.h>

namespace ghost::ui {

void ShaderErrorOverlay::draw(const std::vector<graphics::shader::ShaderError>& errors) {
    if (errors.empty()) {
        return;
    }
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f, viewport->WorkPos.y + 12.0f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.92f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.22f, 0.04f, 0.04f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.55f, 0.08f, 0.08f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.55f, 0.08f, 0.08f, 1.0f));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove;
    if (ImGui::Begin("Shader error", nullptr, flags)) {
        ImGui::TextUnformatted("The last working version keeps running. Fix the file and save: it reloads automatically.");
        ImGui::Separator();
        for (const graphics::shader::ShaderError& error : errors) {
            ImGui::PushID(error.pipeline.c_str());
            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "%s", error.pipeline.c_str());
            if (!error.file.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "%s(%d): %s", error.file.c_str(), error.line, error.message.c_str());
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "%s", error.message.c_str());
            }
            if (ImGui::TreeNode("Full compiler output")) {
                ImGui::TextUnformatted(error.diagnostics.c_str());
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
}

} // namespace ghost::ui
