// TechniquePanel: categories as collapsing headers, one widget per Param type.
#include "UI/TechniquePanel.h"

#include "Graphics/TechniqueRuntime/Param.h"
#include "Graphics/TechniqueRuntime/Technique.h"
#include "Graphics/TechniqueRuntime/TechniqueManager.h"

#include <map>
#include <string>
#include <vector>

#include <imgui.h>

namespace ghost::ui {

namespace {

using graphics::techniques::ParamBase;
using graphics::techniques::Technique;

void drawParam(ParamBase& param) {
    switch (param.type()) {
    case ParamBase::Type::Bool: ImGui::Checkbox(param.label().c_str(), static_cast<bool*>(param.data())); break;
    case ParamBase::Type::Int:
        ImGui::SliderInt(param.label().c_str(), static_cast<int*>(param.data()), static_cast<int>(param.minimum()), static_cast<int>(param.maximum()));
        break;
    case ParamBase::Type::Float:
        ImGui::SliderFloat(param.label().c_str(), static_cast<float*>(param.data()), param.minimum(), param.maximum());
        break;
    case ParamBase::Type::Color: ImGui::ColorEdit3(param.label().c_str(), static_cast<float*>(param.data())); break;
    }
}

} // namespace

void TechniquePanel::draw(graphics::techniques::TechniqueManager& techniques, bool* open) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 340.0f, viewport->WorkPos.y + 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330.0f, 0.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Techniques (F2)", open)) {
        ImGui::End();
        return;
    }
    std::map<std::string, std::vector<Technique*>> byCategory;
    for (Technique* technique : techniques.techniques()) {
        byCategory[technique->category()].push_back(technique);
    }
    if (byCategory.empty()) {
        ImGui::TextWrapped("No techniques yet. Create one with: new_technique.bat <Category> <Name>");
    }
    for (const auto& [category, list] : byCategory) {
        if (!ImGui::CollapsingHeader(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            continue;
        }
        for (Technique* technique : list) {
            ImGui::PushID(technique);
            ImGui::Checkbox(technique->name(), &technique->enabled);
            if (technique->enabled && !technique->params().empty()) {
                ImGui::Indent();
                for (ParamBase* param : technique->params()) {
                    drawParam(*param);
                }
                if (ImGui::SmallButton("Reset")) {
                    for (ParamBase* param : technique->params()) {
                        param->reset();
                    }
                }
                ImGui::Unindent();
            }
            ImGui::PopID();
        }
    }
    ImGui::End();
}

} // namespace ghost::ui
