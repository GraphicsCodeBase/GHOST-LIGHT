// Win32 surface creation. The function pointer is fetched here so no other file needs Win32 Vulkan types.
#include "Graphics/Vulkan/Surface.h"

#include "Core/Log.h"
#include "Graphics/Vulkan/VulkanCheck.h"
#include "Platform/Window.h"

#include <windows.h>

#include <vulkan/vulkan_win32.h>

namespace ghost::graphics::vulkan {

Surface::~Surface() {
    destroy();
}

bool Surface::create(VkInstance instance, const platform::Window& window) {
    auto createWin32Surface = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));
    if (!createWin32Surface) {
        core::Log::error("vkCreateWin32SurfaceKHR is unavailable (VK_KHR_win32_surface not enabled?)");
        return false;
    }
    VkWin32SurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    info.hinstance = GetModuleHandleW(nullptr);
    info.hwnd = static_cast<HWND>(window.nativeHandle());
    VK_CHECK(createWin32Surface(instance, &info, nullptr, &m_surface));
    m_instance = instance;
    return true;
}

void Surface::destroy() {
    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
}

} // namespace ghost::graphics::vulkan
