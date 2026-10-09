// On-screen panel listing scene/prefab problems ("file(line): field: message"). Stays until dismissed or the scene loads cleanly.
#pragma once

#include <string>
#include <vector>

namespace ghost::ui {

class SceneErrorOverlay {
public:
    // Draws nothing when both lists are empty. "Dismiss" clears them.
    static void draw(std::vector<std::string>& errors, std::vector<std::string>& warnings);
};

} // namespace ghost::ui
