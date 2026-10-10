// Techniques panel (F2): every compiled-in technique grouped by category, an enable checkbox each, and its Param<T>
// widgets (sliders, checkboxes, color pickers). Changes apply live; the scene file sets the starting values.
#pragma once

namespace ghost::graphics::techniques {
class TechniqueManager;
}

namespace ghost::ui {

class TechniquePanel {
public:
    static void draw(graphics::techniques::TechniqueManager& techniques, bool* open);
};

} // namespace ghost::ui
