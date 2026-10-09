// Dear ImGui (docking branch) on GLFW + Vulkan dynamic rendering. The window layout is saved in User/imgui.ini.
#pragma once

#include <string>

#include <volk.h>

namespace ghost::platform {
class Window;
}
namespace ghost::graphics {
class Renderer;
}

namespace ghost::ui {

class ImGuiLayer {
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();
    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    // persistLayout = false keeps ImGui from writing User/imgui.ini (smoke test).
    bool initialize(platform::Window& window, graphics::Renderer& renderer, bool persistLayout);
    // The caller makes sure the GPU is idle first (ImGui's Vulkan objects may still be in use).
    void shutdown();

    // Starts the ImGui frame and tells Input whether ImGui wants the keyboard / mouse this frame.
    void beginFrame();
    // Finalizes ImGui's draw lists. Call after all widgets, before the renderer records the frame.
    void endFrame();
    // Records ImGui into the swapchain rendering scope (called by the renderer's overlay hook).
    void record(VkCommandBuffer cmd);

private:
    platform::Window* m_window = nullptr;
    std::string m_iniPath;
    VkFormat m_colorFormat = VK_FORMAT_UNDEFINED; // must outlive ImGui_ImplVulkan_Init (it keeps a pointer)
    bool m_initialized = false;
};

} // namespace ghost::ui
