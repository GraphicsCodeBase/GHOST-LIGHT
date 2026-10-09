// Frame orchestration: owns the Vulkan context, swapchain, shaders and passes; records and submits each frame. App calls it once per frame.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <volk.h>

namespace ghost::platform {
class Window;
}

namespace ghost::graphics::vulkan {
class Instance;
class Surface;
class Device;
class Swapchain;
class FrameScheduler;
class BindlessDescriptors;
class DeletionQueue;
} // namespace ghost::graphics::vulkan

namespace ghost::graphics::shader {
class ShaderCompiler;
class PipelineLibrary;
} // namespace ghost::graphics::shader

namespace ghost::graphics::passes {
class SplashPass;
}

namespace ghost::graphics {

class Renderer {
public:
    struct Desc {
        bool validation = false;
        bool vsync = true;
    };
    // Records extra drawing (the UI) into the final swapchain rendering scope.
    using OverlayRecorder = std::function<void(VkCommandBuffer)>;

    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Returns false after logging a clear reason (no RTX GPU, driver too old, ...).
    bool initialize(platform::Window& window, const Desc& desc);
    void shutdown();
    void waitIdle() const;

    // Renders and presents one frame. Handles minimized windows, resizes and shader hot reload on its own.
    void renderFrame(double timeSeconds, const OverlayRecorder& overlay = {});

    bool validationActive() const;
    std::string gpuName() const;

    // Access for the UI layer and tests.
    const vulkan::Instance& instance() const { return *m_instance; }
    const vulkan::Device& device() const { return *m_device; }
    VkFormat swapchainFormat() const;
    uint32_t swapchainImageCount() const;
    shader::PipelineLibrary& pipelines() { return *m_pipelines; }

private:
    bool syncSwapchainWithWindow();

    platform::Window* m_window = nullptr;
    std::unique_ptr<vulkan::Instance> m_instance;
    std::unique_ptr<vulkan::Surface> m_surface;
    std::unique_ptr<vulkan::Device> m_device;
    std::unique_ptr<vulkan::FrameScheduler> m_frames;
    std::unique_ptr<vulkan::Swapchain> m_swapchain;
    std::unique_ptr<vulkan::DeletionQueue> m_deletionQueue;
    std::unique_ptr<vulkan::BindlessDescriptors> m_bindless;
    std::unique_ptr<shader::ShaderCompiler> m_shaderCompiler;
    std::unique_ptr<shader::PipelineLibrary> m_pipelines;
    std::unique_ptr<passes::SplashPass> m_splash;
    bool m_swapchainOutdated = false;
    bool m_initialized = false;
};

} // namespace ghost::graphics
