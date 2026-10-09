// On-screen panel listing shader compile errors (file, line, message) while the last working shader keeps running.
#pragma once

#include "Graphics/ShaderCompiler/PipelineLibrary.h"

#include <vector>

namespace ghost::ui {

class ShaderErrorOverlay {
public:
    // Draws nothing when there are no errors.
    static void draw(const std::vector<graphics::shader::ShaderError>& errors);
};

} // namespace ghost::ui
