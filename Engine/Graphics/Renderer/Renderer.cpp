// Renderer startup/shutdown order and the per-frame cycle: hot reload -> acquire -> record -> submit -> present.
#include "Graphics/Renderer/Renderer.h"

#include "Core/Log.h"
#include "Core/Paths.h"
#include "Graphics/Passes/SplashPass.h"
#include "Graphics/ShaderCompiler/PipelineLibrary.h"
#include "Graphics/ShaderCompiler/ShaderCompiler.h"
#include "Graphics/Vulkan/BindlessDescriptors.h"
#include "Graphics/Vulkan/DebugUtils.h"
#include "Graphics/Vulkan/DeletionQueue.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/FrameScheduler.h"
#include "Graphics/Vulkan/GpuCrashReporter.h"
#include "Graphics/Vulkan/Instance.h"
#include "Graphics/Vulkan/Surface.h"
#include "Graphics/Vulkan/Swapchain.h"
#include "Platform/Window.h"

namespace ghost::graphics {

namespace {

void imageBarrier(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                  VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess) {
    VkImageMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    barrier.srcStageMask = srcStage;
    barrier.srcAccessMask = srcAccess;
    barrier.dstStageMask = dstStage;
    barrier.dstAccessMask = dstAccess;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(cmd, &dependency);
}

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

    m_splash = std::make_unique<passes::SplashPass>();
    m_splash->initialize(*m_pipelines, m_swapchain->format());

    m_initialized = true;
    return true;
}

void Renderer::waitIdle() const {
    if (m_device) {
        m_device->waitIdle();
    }
}

void Renderer::shutdown() {
    waitIdle();
    // Reverse creation order: everything that lives on the device goes before the device, the surface before the instance.
    m_splash.reset();
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

void Renderer::renderFrame(double timeSeconds, const OverlayRecorder& overlay) {
    if (!m_initialized) {
        return;
    }
    // Retire objects the GPU is done with, then pick up edited shaders (replacements retire after the last submitted frame).
    m_deletionQueue->flush(m_frames->completedFrame());
    m_pipelines->update(m_frames->frameNumber());
    if (!syncSwapchainWithWindow()) {
        return;
    }

    VkCommandBuffer cmd = m_frames->beginFrame();
    uint32_t imageIndex = 0;
    if (!m_swapchain->acquire(m_frames->imageAvailable(), imageIndex)) {
        m_frames->cancelFrame();
        m_swapchainOutdated = true;
        return;
    }

    const VkImage image = m_swapchain->image(imageIndex);
    const VkExtent2D extent = m_swapchain->extent();
    imageBarrier(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                 VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

    VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colorAttachment.imageView = m_swapchain->view(imageIndex);
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    VkRenderingInfo rendering{VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering.renderArea = {{0, 0}, extent};
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachments = &colorAttachment;

    vulkan::DebugUtils::beginLabel(cmd, "Splash + UI");
    vulkan::GpuCrashReporter::checkpoint(cmd, "Splash + UI");
    vkCmdBeginRendering(cmd, &rendering);
    m_bindless->bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS);
    m_splash->record(cmd, extent, static_cast<float>(timeSeconds));
    if (overlay) {
        overlay(cmd);
    }
    vkCmdEndRendering(cmd);
    vulkan::DebugUtils::endLabel(cmd);

    imageBarrier(cmd, image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                 VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE);

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

VkFormat Renderer::swapchainFormat() const {
    return m_swapchain->format();
}

uint32_t Renderer::swapchainImageCount() const {
    return m_swapchain->imageCount();
}

} // namespace ghost::graphics
