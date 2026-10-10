// Texture viewer panel (F3): pick any render graph texture to show on screen, its channels and value range, with
// presets for depth, normals, motion vectors and HDR color. Drawing is done by Graphics/Passes/TextureViewerPass.
#pragma once

namespace ghost::graphics {
class Renderer;
}

namespace ghost::ui {

class TextureViewerPanel {
public:
    static void draw(graphics::Renderer& renderer, bool* open);
};

} // namespace ghost::ui
