// ImGui context, style, GLFW/Vulkan backends and per-frame begin/end/record.
#include "UI/ImGuiLayer.h"

#include "Core/Log.h"
#include "Core/Paths.h"
#include "Graphics/Renderer/Renderer.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/Instance.h"
#include "Platform/Window.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace ghost::ui {

namespace {

void checkVulkanResult(VkResult result) {
    if (result < 0) {
        core::Log::error("ImGui Vulkan backend error: VkResult {}", static_cast<int>(result));
    }
}

// A calm dark theme with the ghost-light amber as accent.
void applyStyle(float scale) {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowBorderSize = 0.0f;
    ImVec4* colors = style.Colors;
    const ImVec4 amber(1.00f, 0.66f, 0.30f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.08f, 0.94f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.12f, 0.08f, 1.00f);
    colors[ImGuiCol_CheckMark] = amber;
    colors[ImGuiCol_SliderGrab] = amber;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.76f, 0.45f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.36f, 0.25f, 0.13f, 0.60f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.48f, 0.33f, 0.17f, 0.80f);
    style.ScaleAllSizes(scale);
    style.FontScaleDpi = scale;
}

} // namespace

ImGuiLayer::~ImGuiLayer() {
    shutdown();
}

bool ImGuiLayer::initialize(platform::Window& window, graphics::Renderer& renderer, bool persistLayout) {
    m_window = &window;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    if (persistLayout) {
        m_iniPath = core::Paths::toUtf8(core::Paths::user() / "imgui.ini");
        io.IniFilename = m_iniPath.c_str();
    } else {
        io.IniFilename = nullptr;
    }

    float scaleX = 1.0f;
    float scaleY = 1.0f;
    glfwGetWindowContentScale(window.handle(), &scaleX, &scaleY);
    applyStyle(scaleY);

    // install_callbacks = true chains to the window's own callbacks, so engine input keeps working.
    if (!ImGui_ImplGlfw_InitForVulkan(window.handle(), true)) {
        core::Log::error("ImGui GLFW backend failed to initialize");
        return false;
    }

    const graphics::vulkan::Device& device = renderer.device();
    m_colorFormat = renderer.swapchainFormat();
    ImGui_ImplVulkan_InitInfo info{};
    info.ApiVersion = VK_API_VERSION_1_3;
    info.Instance = renderer.instance().handle();
    info.PhysicalDevice = device.physicalDevice();
    info.Device = device.handle();
    info.QueueFamily = device.graphicsQueueFamily();
    info.Queue = device.graphicsQueue();
    info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE + 64; // room for texture-viewer images later
    info.MinImageCount = 2;
    info.ImageCount = renderer.swapchainImageCount();
    info.UseDynamicRendering = true;
    info.PipelineInfoMain.PipelineRenderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_colorFormat;
    info.CheckVkResultFn = checkVulkanResult;
    if (!ImGui_ImplVulkan_Init(&info)) {
        core::Log::error("ImGui Vulkan backend failed to initialize");
        return false;
    }
    m_initialized = true;
    return true;
}

void ImGuiLayer::shutdown() {
    if (!m_initialized) {
        return;
    }
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

void ImGuiLayer::beginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // Panels can dock to the screen edges; the middle stays see-through so the scene is visible.
    ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
    const ImGuiIO& io = ImGui::GetIO();
    // Keyboard: only while typing into a text field. A focused panel must not swallow WASD or the F-key shortcuts.
    // Mouse: while the cursor is over a panel (clicks and drags go to the UI, not the camera).
    m_window->input().setUiCapture(io.WantTextInput, io.WantCaptureMouse);
}

void ImGuiLayer::endFrame() {
    ImGui::Render();
}

void ImGuiLayer::record(VkCommandBuffer cmd) {
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

} // namespace ghost::ui
