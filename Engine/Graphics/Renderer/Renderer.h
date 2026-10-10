// Frame orchestration: owns the Vulkan context, swapchain, shaders, GPU scene, render graph and passes; records and
// submits each frame. App calls it once per frame.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

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
class GpuBuffer;
} // namespace ghost::graphics::vulkan

namespace ghost::graphics::shader {
class ShaderCompiler;
class PipelineLibrary;
} // namespace ghost::graphics::shader

namespace ghost::graphics::rendergraph {
class RenderGraph;
class GpuTimers;
} // namespace ghost::graphics::rendergraph

namespace ghost::graphics::scene {
class GpuScene;
}

namespace ghost::graphics::raytracing {
class SceneAccelerationStructures;
}

namespace ghost::graphics::techniques {
class TechniqueManager;
}

namespace ghost::graphics::passes {
class GBufferPass;
class LightingPass;
class ReferencePathTracerPass;
class TonemapPass;
} // namespace ghost::graphics::passes

namespace ghost::graphics {

class Renderer {
public:
    // Raster: G-buffer + placeholder lighting + enabled techniques (by stage).
    // PathTraced: the naive reference path tracer, accumulating while nothing changes.
    enum class Mode { Raster, PathTraced };

    struct Desc {
        bool validation = false;
        bool vsync = true;
    };
    // Records extra drawing (the UI) on top of the final image, inside the "UI" pass.
    using OverlayRecorder = std::function<void(VkCommandBuffer)>;
    // Receives a finished frame: width, height, BGRA8 pixels (tightly packed rows).
    using CaptureCallback = std::function<void(uint32_t width, uint32_t height, const uint8_t* bgra)>;

    struct PassTiming {
        std::string name;
        double milliseconds = 0.0;
    };

    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Returns false after logging a clear reason (no RTX GPU, driver too old, ...).
    bool initialize(platform::Window& window, const Desc& desc);
    void shutdown();
    void waitIdle() const;

    // Renders and presents one frame of the GPU scene as last filled (gpuScene()). Handles minimized windows, resizes
    // and shader hot reload on its own.
    void renderFrame(const OverlayRecorder& overlay = {});
    // Captures the next rendered frame (UI included); the callback runs once the GPU has finished it.
    void requestCapture(CaptureCallback callback);

    bool validationActive() const;
    std::string gpuName() const;
    std::vector<PassTiming> gpuTimings() const;
    double gpuFrameMilliseconds() const;

    // Access for the UI layer and tests.
    const vulkan::Instance& instance() const { return *m_instance; }
    const vulkan::Device& device() const { return *m_device; }
    VkFormat swapchainFormat() const;
    uint32_t swapchainImageCount() const;
    shader::PipelineLibrary& pipelines() { return *m_pipelines; }
    rendergraph::RenderGraph& renderGraph() { return *m_graph; }
    scene::GpuScene& gpuScene() { return *m_gpuScene; }
    const raytracing::SceneAccelerationStructures& accelerationStructures() const { return *m_accelerationStructures; }
    passes::ReferencePathTracerPass& pathTracer() { return *m_pathTracer; }
    // Every registered technique; toggling one rebuilds the render graph before the next frame.
    techniques::TechniqueManager& techniques() { return *m_techniques; }

    void setMode(Mode mode);
    Mode mode() const { return m_mode; }

private:
    bool syncSwapchainWithWindow();
    void buildRenderGraph();
    void recordCapture(VkCommandBuffer cmd, VkImage image);
    void deliverCaptureIfReady(bool gpuIdle);

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
    std::unique_ptr<rendergraph::RenderGraph> m_graph;
    std::unique_ptr<rendergraph::GpuTimers> m_timers;
    std::unique_ptr<scene::GpuScene> m_gpuScene;
    std::unique_ptr<raytracing::SceneAccelerationStructures> m_accelerationStructures;
    std::unique_ptr<passes::GBufferPass> m_gbuffer;
    std::unique_ptr<passes::LightingPass> m_lighting;
    std::unique_ptr<passes::ReferencePathTracerPass> m_pathTracer;
    std::unique_ptr<passes::TonemapPass> m_tonemap;
    std::unique_ptr<techniques::TechniqueManager> m_techniques;
    Mode m_mode = Mode::Raster;
    bool m_graphDirty = false;

    OverlayRecorder m_overlay;
    // Frame capture: requested -> recorded into a frame -> delivered when that frame completes on the GPU.
    CaptureCallback m_captureRequest;
    CaptureCallback m_capturePending;
    std::unique_ptr<vulkan::GpuBuffer> m_captureBuffer;
    VkExtent2D m_captureExtent{};
    uint64_t m_captureFrame = 0;

    bool m_swapchainOutdated = false;
    bool m_initialized = false;
};

} // namespace ghost::graphics
