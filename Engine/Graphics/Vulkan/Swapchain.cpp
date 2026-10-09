// Swapchain creation (vk-bootstrap), acquire/present with out-of-date handling, per-image present semaphores.
#include "Graphics/Vulkan/Swapchain.h"

#include "Core/Log.h"
#include "Graphics/Vulkan/DebugUtils.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/VulkanCheck.h"

#include <string>

#include <VkBootstrap.h>

namespace ghost::graphics::vulkan {

Swapchain::~Swapchain() {
    destroy();
}

bool Swapchain::create(const Device& device, VkSurfaceKHR surface, uint32_t width, uint32_t height, bool vsync) {
    m_device = &device;
    m_surface = surface;
    m_vsync = vsync;
    return build(width, height, VK_NULL_HANDLE);
}

bool Swapchain::recreate(uint32_t width, uint32_t height) {
    destroyImageResources();
    const VkSwapchainKHR old = m_swapchain;
    const bool ok = build(width, height, old);
    vkDestroySwapchainKHR(m_device->handle(), old, nullptr);
    return ok;
}

bool Swapchain::build(uint32_t width, uint32_t height, VkSwapchainKHR oldSwapchain) {
    vkb::SwapchainBuilder builder(m_device->bootstrap(), m_surface);
    // UNORM, not SRGB: the tonemap pass writes display-encoded values itself, and ImGui's colors are already sRGB.
    builder.set_desired_format({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_extent(width, height)
        .set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        .set_old_swapchain(oldSwapchain);
    if (m_vsync) {
        builder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR);
    } else {
        builder.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .add_fallback_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
            .add_fallback_present_mode(VK_PRESENT_MODE_FIFO_KHR);
    }

    auto built = builder.build();
    if (!built) {
        core::Log::error("Could not create the swapchain ({}x{}): {}", width, height, built.error().message());
        m_swapchain = VK_NULL_HANDLE;
        return false;
    }
    vkb::Swapchain swapchain = built.value();
    m_swapchain = swapchain.swapchain;
    m_format = swapchain.image_format;
    m_extent = swapchain.extent;
    m_images = swapchain.get_images().value();
    m_views = swapchain.get_image_views().value();
    DebugUtils::setName(m_swapchain, "Swapchain");

    VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    m_renderFinished.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); ++i) {
        VK_CHECK(vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_renderFinished[i]));
        const std::string index = std::to_string(i);
        DebugUtils::setName(m_images[i], "Swapchain.Image" + index);
        DebugUtils::setName(m_views[i], "Swapchain.View" + index);
        DebugUtils::setName(m_renderFinished[i], "Swapchain.RenderFinished" + index);
    }
    return true;
}

bool Swapchain::acquire(VkSemaphore imageAvailable, uint32_t& imageIndex) {
    const VkResult result = vkAcquireNextImageKHR(m_device->handle(), m_swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return false;
    }
    VK_CHECK(result); // VK_SUBOPTIMAL_KHR still acquired an image: render it, recreate after present.
    return true;
}

bool Swapchain::present(VkQueue queue, uint32_t imageIndex) {
    VkPresentInfoKHR info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &m_renderFinished[imageIndex];
    info.swapchainCount = 1;
    info.pSwapchains = &m_swapchain;
    info.pImageIndices = &imageIndex;
    const VkResult result = vkQueuePresentKHR(queue, &info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return false;
    }
    VK_CHECK(result);
    return true;
}

void Swapchain::destroyImageResources() {
    if (!m_device) {
        return;
    }
    for (VkImageView view : m_views) {
        vkDestroyImageView(m_device->handle(), view, nullptr);
    }
    for (VkSemaphore semaphore : m_renderFinished) {
        vkDestroySemaphore(m_device->handle(), semaphore, nullptr);
    }
    m_views.clear();
    m_renderFinished.clear();
    m_images.clear();
}

void Swapchain::destroy() {
    destroyImageResources();
    if (m_device && m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device->handle(), m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

} // namespace ghost::graphics::vulkan
