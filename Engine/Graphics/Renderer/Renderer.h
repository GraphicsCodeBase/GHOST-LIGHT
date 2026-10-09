// Frame orchestration: owns the Vulkan context and swapchain, records and submits each frame. App calls it once per frame.
#pragma once

#include <memory>
#include <string>

namespace ghost::platform {
class Window;
}

namespace ghost::graphics::vulkan {
class Instance;
class Surface;
class Device;
class Swapchain;
class FrameScheduler;
} // namespace ghost::graphics::vulkan

namespace ghost::graphics {

class Renderer {
public:
    struct Desc {
        bool validation = false;
        bool vsync = true;
    };

    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Returns false after logging a clear reason (no RTX GPU, driver too old, ...).
    bool initialize(platform::Window& window, const Desc& desc);
    void shutdown();

    // Renders and presents one frame. Handles minimized windows and resizes on its own.
    void renderFrame(double timeSeconds);

    bool validationActive() const;
    std::string gpuName() const;

private:
    bool syncSwapchainWithWindow();

    platform::Window* m_window = nullptr;
    std::unique_ptr<vulkan::Instance> m_instance;
    std::unique_ptr<vulkan::Surface> m_surface;
    std::unique_ptr<vulkan::Device> m_device;
    std::unique_ptr<vulkan::Swapchain> m_swapchain;
    std::unique_ptr<vulkan::FrameScheduler> m_frames;
    bool m_swapchainOutdated = false;
    bool m_initialized = false;
};

} // namespace ghost::graphics
