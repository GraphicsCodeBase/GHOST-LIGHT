// The window's Vulkan surface (VK_KHR_win32_surface), created from the platform window's native handle.
#pragma once

#include <volk.h>

namespace ghost::platform {
class Window;
}

namespace ghost::graphics::vulkan {

class Surface {
public:
    Surface() = default;
    ~Surface();
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    bool create(VkInstance instance, const platform::Window& window);
    void destroy();

    VkSurfaceKHR handle() const { return m_surface; }

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};

} // namespace ghost::graphics::vulkan
