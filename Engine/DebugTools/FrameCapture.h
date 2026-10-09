// Saves rendered frames as PNG: F12 writes Captures/GhostLight_<date>_<time>.png (UI included).
#pragma once

#include <filesystem>

namespace ghost::graphics {
class Renderer;
}

namespace ghost::debugtools {

class FrameCapture {
public:
    // Captures the next frame; the file is written once the GPU has finished it (a couple of frames later).
    static void requestPng(graphics::Renderer& renderer, std::filesystem::path path);
    // Captures/GhostLight_YYYYMMDD_HHMMSS.png (Captures/ is gitignored).
    static std::filesystem::path defaultPath();
};

} // namespace ghost::debugtools
