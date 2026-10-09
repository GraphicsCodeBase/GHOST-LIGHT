// FrameCapture: BGRA swapchain readback -> RGBA -> PNG on disk.
#include "DebugTools/FrameCapture.h"

#include "Core/Log.h"
#include "Core/Paths.h"
#include "DebugTools/PngWriter.h"
#include "Graphics/Renderer/Renderer.h"

#include <chrono>
#include <ctime>
#include <format>
#include <vector>

namespace ghost::debugtools {

void FrameCapture::requestPng(graphics::Renderer& renderer, std::filesystem::path path) {
    renderer.requestCapture([path = std::move(path)](uint32_t width, uint32_t height, const uint8_t* bgra) {
        std::vector<uint8_t> rgba(static_cast<size_t>(width) * height * 4);
        for (size_t i = 0; i < rgba.size(); i += 4) {
            rgba[i + 0] = bgra[i + 2];
            rgba[i + 1] = bgra[i + 1];
            rgba[i + 2] = bgra[i + 0];
            rgba[i + 3] = 255;
        }
        std::string error;
        if (PngWriter::writeRgba8(path, width, height, rgba.data(), error)) {
            core::Log::info("Screenshot saved: {}", core::Paths::display(path));
        } else {
            core::Log::warning("Screenshot failed: {}", error);
        }
    });
}

std::filesystem::path FrameCapture::defaultPath() {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
    localtime_s(&local, &now);
    const std::string name = std::format("GhostLight_{:04}{:02}{:02}_{:02}{:02}{:02}.png", local.tm_year + 1900, local.tm_mon + 1,
                                         local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
    return core::Paths::root() / "Captures" / name;
}

} // namespace ghost::debugtools
