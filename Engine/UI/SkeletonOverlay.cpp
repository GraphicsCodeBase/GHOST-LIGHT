// Skeleton overlay: projects joint segments to the screen and draws them on ImGui's background layer (no depth test,
// so the skeleton stays visible inside the mesh; panels still draw on top).
#include "UI/SkeletonOverlay.h"

#include <optional>

#include <glm/vec4.hpp>
#include <imgui.h>

namespace ghost::ui {

namespace {

// World point -> screen pixels, or nothing when it is behind the camera. The projection already flips y for Vulkan,
// so NDC y = -1 is the top of the screen.
std::optional<ImVec2> toScreen(const glm::vec3& point, const glm::mat4& viewProjection, const ImVec2& origin, const ImVec2& size) {
    const glm::vec4 clip = viewProjection * glm::vec4(point, 1.0f);
    if (clip.w <= 1e-4f) {
        return std::nullopt;
    }
    const float x = clip.x / clip.w;
    const float y = clip.y / clip.w;
    return ImVec2(origin.x + (x * 0.5f + 0.5f) * size.x, origin.y + (y * 0.5f + 0.5f) * size.y);
}

} // namespace

void SkeletonOverlay::draw(const std::vector<std::pair<glm::vec3, glm::vec3>>& lines, const glm::mat4& viewProjection) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    const ImU32 boneColor = IM_COL32(255, 200, 40, 255);
    const ImU32 jointColor = IM_COL32(255, 80, 40, 255);
    for (const auto& [from, to] : lines) {
        const auto a = toScreen(from, viewProjection, viewport->Pos, viewport->Size);
        const auto b = toScreen(to, viewProjection, viewport->Pos, viewport->Size);
        if (!a || !b) {
            continue;
        }
        drawList->AddLine(*a, *b, boneColor, 2.0f);
        drawList->AddCircleFilled(*b, 3.0f, jointColor);
    }
}

} // namespace ghost::ui
