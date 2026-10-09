// Renderer startup/shutdown order and the per-frame cycle: hot reload -> acquire -> GPU scene upload -> render graph ->
// submit -> present.
#include "Graphics/Renderer/Renderer.h"

#include "Core/Log.h"
#include "Core/Paths.h"
#include "Graphics/GpuScene/GpuScene.h"
#include "Graphics/Passes/GBufferPass.h"
#include "Graphics/Passes/LightingPass.h"
#include "Graphics/Passes/TonemapPass.h"
#include "Graphics/RenderGraph/GpuTimers.h"
#include "Graphics/RenderGraph/RenderGraph.h"
#include "Graphics/ShaderCompiler/PipelineLibrary.h"
#include "Graphics/ShaderCompiler/ShaderCompiler.h"
#include "Graphics/Vulkan/BindlessDescriptors.h"
#include "Graphics/Vulkan/DeletionQueue.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/FrameScheduler.h"
#include "Graphics/Vulkan/GpuBuffer.h"
#include "Graphics/Vulkan/Instance.h"
#include "Graphics/Vulkan/Surface.h"
#include "Graphics/Vulkan/Swapchain.h"
#include "Platform/Window.h"

namespace ghost::graphics {

namespace {

constexpr const char* kSwapchain = "swapchain";

} // namespace

Renderer::Renderer() = default;

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize(platform::Window& window, const Desc& desc) {
    m_window = &window;
    m_instance = std::make_unique<vulkan::Instance>();
    if (!m_instance->create(desc.validation)) {
        return false;
    }
    m_surface = std::make_unique<vulkan::Surface>();
    if (!m_surface->create(m_instance->handle(), window)) {
        return false;
    }
    m_device = std::make_unique<vulkan::Device>();
    if (!m_device->create(*m_instance, m_surface->handle())) {
        return false;
    }
    m_frames = std::make_unique<vulkan::FrameScheduler>();
    if (!m_frames->create(*m_device)) {
        return false;
    }
    const glm::ivec2 size = window.framebufferSize();
    m_swapchain = std::make_unique<vulkan::Swapchain>();
    if (!m_swapchain->create(*m_device, m_surface->handle(), static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y), desc.vsync)) {
        return false;
    }
    core::Log::info("Swapchain {}x{}, {} images, vsync {}", m_swapchain->extent().width, m_swapchain->extent().height,
                    m_swapchain->imageCount(), desc.vsync ? "on" : "off");

    m_deletionQueue = std::make_unique<vulkan::DeletionQueue>();
    m_bindless = std::make_unique<vulkan::BindlessDescriptors>();
    if (!m_bindless->create(*m_device)) {
        return false;
    }
    m_shaderCompiler = std::make_unique<shader::ShaderCompiler>();
    if (!m_shaderCompiler->initialize({core::Paths::shaderLibrary()})) {
        return false;
    }
    m_pipelines = std::make_unique<shader::PipelineLibrary>();
    m_pipelines->initialize(*m_device, *m_bindless, *m_deletionQueue, *m_shaderCompiler);

    m_graph = std::make_unique<rendergraph::RenderGraph>();
    m_graph->initialize(*m_device, *m_bindless, *m_deletionQueue);
    m_timers = std::make_unique<rendergraph::GpuTimers>();
    m_timers->create(*m_device, vulkan::FrameScheduler::kFramesInFlight);

    m_gpuScene = std::make_unique<scene::GpuScene>();
    m_gpuScene->initialize(*m_device, *m_bindless, *m_deletionQueue);
    m_gbuffer = std::make_unique<passes::GBufferPass>();
    m_gbuffer->initialize(*m_pipelines);
    m_lighting = std::make_unique<passes::LightingPass>();
    m_lighting->initialize(*m_pipelines);
    m_tonemap = std::make_unique<passes::TonemapPass>();
    m_tonemap->initialize(*m_pipelines, m_swapchain->format());
    buildRenderGraph();

    m_initialized = true;
    return true;
}

void Renderer::buildRenderGraph() {
    rendergraph::RenderGraph& graph = *m_graph;
    graph.reset();
    graph.declareExternal(kSwapchain, m_swapchain->format());
    m_gbuffer->addTo(graph, *m_gpuScene);
    m_lighting->addTo(graph, *m_gpuScene);
    m_tonemap->addTo(graph, kSwapchain);
    graph.addPass(
        "UI", rendergraph::PassKind::Raster,
        [](rendergraph::PassBuilder& builder) { builder.colorAttachment(kSwapchain, rendergraph::LoadOp::Load); },
        [this](rendergraph::PassContext& ctx) {
            if (m_overlay) {
                m_overlay(ctx.cmd());
            }
        });
    auto source = std::make_shared<rendergraph::TextureHandle>();
    graph.addPass(
        "Capture", rendergraph::PassKind::Transfer,
        [source](rendergraph::PassBuilder& builder) { *source = builder.copySource(kSwapchain); },
        [this, source](rendergraph::PassContext& ctx) { recordCapture(ctx.cmd(), ctx.image(*source)); });
    graph.setPassEnabled("Capture", false);
}

void Renderer::waitIdle() const {
    if (m_device) {
        m_device->waitIdle();
    }
}

void Renderer::shutdown() {
    waitIdle();
    deliverCaptureIfReady(/*gpuIdle*/ true);
    // Reverse creation order: everything that lives on the device goes before the device, the surface before the instance.
    m_tonemap.reset();
    m_lighting.reset();
    m_gbuffer.reset();
    m_timers.reset();
    m_graph.reset();
    m_gpuScene.reset(); // its retired buffers/textures sit in the deletion queue, flushed below
    m_captureBuffer.reset();
    m_pipelines.reset();
    m_shaderCompiler.reset();
    if (m_deletionQueue) {
        m_deletionQueue->flushAll();
    }
    m_deletionQueue.reset();
    m_bindless.reset();
    m_swapchain.reset();
    m_frames.reset();
    m_device.reset();
    m_surface.reset();
    m_instance.reset();
    m_initialized = false;
}

bool Renderer::syncSwapchainWithWindow() {
    const glm::ivec2 size = m_window->framebufferSize();
    if (size.x == 0 || size.y == 0) {
        return false; // minimized
    }
    const VkExtent2D extent = m_swapchain->extent();
    if (m_swapchainOutdated || extent.width != static_cast<uint32_t>(size.x) || extent.height != static_cast<uint32_t>(size.y)) {
        m_device->waitIdle();
        if (!m_swapchain->recreate(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y))) {
            return false;
        }
        m_swapchainOutdated = false;
        core::Log::info("Swapchain recreated at {}x{}", m_swapchain->extent().width, m_swapchain->extent().height);
    }
    return true;
}

void Renderer::requestCapture(CaptureCallback callback) {
    m_captureRequest = std::move(callback);
}

void Renderer::recordCapture(VkCommandBuffer cmd, VkImage image) {
    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {m_captureExtent.width, m_captureExtent.height, 1};
    vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_captureBuffer->handle(), 1, &region);

    // Make the copy visible to the CPU once the frame's timeline value is reached.
    VkMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
    VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.memoryBarrierCount = 1;
    dependency.pMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(cmd, &dependency);
}

void Renderer::deliverCaptureIfReady(bool gpuIdle) {
    if (!m_capturePending || !m_frames) {
        return;
    }
    if (!gpuIdle && m_frames->completedFrame() < m_captureFrame) {
        return;
    }
    m_captureBuffer->invalidate();
    m_capturePending(m_captureExtent.width, m_captureExtent.height, static_cast<const uint8_t*>(m_captureBuffer->mapped()));
    m_capturePending = nullptr;
}

void Renderer::renderFrame(const OverlayRecorder& overlay) {
    if (!m_initialized) {
        return;
    }
    // Retire objects the GPU is done with, then pick up edited shaders (replacements retire after the last submitted frame).
    m_deletionQueue->flush(m_frames->completedFrame());
    deliverCaptureIfReady(false);
    m_pipelines->update(m_frames->frameNumber());
    if (!syncSwapchainWithWindow()) {
        return;
    }

    VkCommandBuffer cmd = m_frames->beginFrame();
    m_timers->beginFrame(m_frames->slot()); // this slot's previous frame is done: its timings are ready
    uint32_t imageIndex = 0;
    if (!m_swapchain->acquire(m_frames->imageAvailable(), imageIndex)) {
        m_frames->cancelFrame();
        m_swapchainOutdated = true;
        return;
    }

    const VkExtent2D extent = m_swapchain->extent();
    // This slot's previous frame has finished, so its per-frame scene buffers can be rewritten.
    const uint64_t thisFrame = m_frames->frameNumber() + 1; // the timeline value this submit will signal
    m_gpuScene->prepareFrame(m_frames->slot(), thisFrame, extent, thisFrame);
    m_tonemap->exposure = m_gpuScene->exposure();
    m_graph->compile(extent, m_frames->frameNumber());
    m_graph->bindExternal(kSwapchain, m_swapchain->image(imageIndex), m_swapchain->view(imageIndex), extent, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    m_overlay = overlay;

    const bool capture = m_captureRequest && !m_capturePending;
    if (capture) {
        const VkDeviceSize bytes = static_cast<VkDeviceSize>(extent.width) * extent.height * 4;
        if (!m_captureBuffer || m_captureBuffer->size() < bytes) {
            m_captureBuffer = std::make_unique<vulkan::GpuBuffer>();
            m_captureBuffer->create(*m_device, {bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, vulkan::GpuBuffer::Memory::Readback, "FrameCapture"});
        }
        m_captureExtent = extent;
        m_graph->setPassEnabled("Capture", true);
    }

    m_graph->execute(cmd, thisFrame, m_timers.get());

    if (capture) {
        m_graph->setPassEnabled("Capture", false);
        m_capturePending = std::move(m_captureRequest);
        m_captureRequest = nullptr;
        m_captureFrame = thisFrame;
    }
    m_overlay = nullptr;

    m_frames->submit(m_frames->imageAvailable(), m_swapchain->renderFinished(imageIndex));
    if (!m_swapchain->present(m_device->graphicsQueue(), imageIndex)) {
        m_swapchainOutdated = true;
    }
}

bool Renderer::validationActive() const {
    return m_instance && m_instance->validationEnabled();
}

std::string Renderer::gpuName() const {
    return m_device ? m_device->name() : std::string();
}

std::vector<Renderer::PassTiming> Renderer::gpuTimings() const {
    std::vector<PassTiming> timings;
    if (m_timers) {
        for (const auto& timing : m_timers->results()) {
            timings.push_back({timing.name, timing.milliseconds});
        }
    }
    return timings;
}

double Renderer::gpuFrameMilliseconds() const {
    return m_timers ? m_timers->totalMilliseconds() : 0.0;
}

VkFormat Renderer::swapchainFormat() const {
    return m_swapchain->format();
}

uint32_t Renderer::swapchainImageCount() const {
    return m_swapchain->imageCount();
}

} // namespace ghost::graphics
