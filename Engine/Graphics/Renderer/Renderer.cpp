// Renderer startup/shutdown order and the per-frame acquire -> record -> submit -> present cycle.
#include "Graphics/Renderer/Renderer.h"

#include "Core/Log.h"
#include "Graphics/Vulkan/DebugUtils.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/FrameScheduler.h"
#include "Graphics/Vulkan/GpuCrashReporter.h"
#include "Graphics/Vulkan/Instance.h"
#include "Graphics/Vulkan/Surface.h"
#include "Graphics/Vulkan/Swapchain.h"
#include "Platform/Window.h"

#include <cmath>

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
    m_initialized = true;
    return true;
}

void Renderer::shutdown() {
    if (m_device) {
        m_device->waitIdle();
    }
    // Reverse creation order: everything that lives on the device goes before the device, the surface before the instance.
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

void Renderer::renderFrame(double timeSeconds) {
    if (!m_initialized || !syncSwapchainWithWindow()) {
        return;
    }

    VkCommandBuffer cmd = m_frames->beginFrame();
    uint32_t imageIndex = 0;
    if (!m_swapchain->acquire(m_frames->imageAvailable(), imageIndex)) {
        m_frames->cancelFrame();
        m_swapchainOutdated = true;
        return;
    }

    // Placeholder frame until the render graph exists: clear to a slowly breathing ghost-light amber.
    const VkImage image = m_swapchain->image(imageIndex);
    vulkan::DebugUtils::beginLabel(cmd, "Clear");
    vulkan::GpuCrashReporter::checkpoint(cmd, "Clear");
    imageBarrier(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
    const float glow = 0.5f + 0.5f * static_cast<float>(std::sin(timeSeconds * 1.5));
    const VkClearColorValue color{{0.06f + 0.05f * glow, 0.045f + 0.03f * glow, 0.02f, 1.0f}};
    const VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdClearColorImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);
    imageBarrier(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                 VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE);
    vulkan::DebugUtils::endLabel(cmd);

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

} // namespace ghost::graphics
