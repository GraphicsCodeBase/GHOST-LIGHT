// Swapchain: the window's presentable images, recreated on resize, with one "ready to present" semaphore per image.
#pragma once

#include <volk.h>

#include <vector>

namespace ghost::graphics::vulkan {

class Device;

class Swapchain {
public:
    Swapchain() = default;
    ~Swapchain();
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    // vsync: FIFO (capped to the display). Otherwise MAILBOX, falling back to IMMEDIATE, then FIFO.
    bool create(const Device& device, VkSurfaceKHR surface, uint32_t width, uint32_t height, bool vsync);
    // Rebuilds for a new size, reusing the old swapchain. The caller makes sure the GPU no longer uses it.
    bool recreate(uint32_t width, uint32_t height);
    void destroy();

    // Returns false when the swapchain is out of date and must be recreated (no image acquired).
    bool acquire(VkSemaphore imageAvailable, uint32_t& imageIndex);
    // Returns false when the swapchain should be recreated (out of date or suboptimal).
    bool present(VkQueue queue, uint32_t imageIndex);

    VkSemaphore renderFinished(uint32_t imageIndex) const { return m_renderFinished[imageIndex]; }
    VkImage image(uint32_t imageIndex) const { return m_images[imageIndex]; }
    VkImageView view(uint32_t imageIndex) const { return m_views[imageIndex]; }
    VkFormat format() const { return m_format; }
    VkExtent2D extent() const { return m_extent; }
    uint32_t imageCount() const { return static_cast<uint32_t>(m_images.size()); }

private:
    bool build(uint32_t width, uint32_t height, VkSwapchainKHR oldSwapchain);
    void destroyImageResources();

    const Device* m_device = nullptr;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkExtent2D m_extent{};
    bool m_vsync = true;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_views;
    std::vector<VkSemaphore> m_renderFinished;
};

} // namespace ghost::graphics::vulkan
