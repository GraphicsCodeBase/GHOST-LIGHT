// Native OS window (GLFW, no graphics context). The renderer creates its Vulkan surface from it.
#pragma once

#include "Platform/Input.h"

#include <string>

#include <glm/vec2.hpp>

struct GLFWwindow;

namespace ghost::platform {

class Window {
public:
    struct Desc {
        std::string title = "GHOST LIGHT";
        int width = 1920;  // client area, screen coordinates
        int height = 1080;
        bool maximized = false;
    };

    explicit Window(const Desc& desc);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // False when the window could not be created (the reason is logged).
    bool isValid() const { return m_window != nullptr; }

    // Clears last frame's input edges, then processes pending OS events.
    void pollEvents();
    // Blocks until an event arrives. Used while minimized so a hidden engine doesn't burn the GPU.
    void waitEvents();

    bool shouldClose() const;
    void requestClose();

    glm::ivec2 framebufferSize() const; // pixels; 0x0 while minimized
    glm::ivec2 windowSize() const;      // screen coordinates (what UserSettings stores)
    bool isMinimized() const;
    bool isMaximized() const;
    bool isFullscreen() const { return m_fullscreen; }

    void setTitle(const std::string& title);
    // Hides the cursor and reports unbounded mouse movement (camera control).
    void setCursorCaptured(bool captured);
    bool isCursorCaptured() const { return m_cursorCaptured; }
    // Borderless fullscreen on the current monitor, toggling back to the previous window placement.
    void toggleFullscreen();

    Input& input() { return m_input; }
    const Input& input() const { return m_input; }
    GLFWwindow* handle() const { return m_window; }

private:
    struct Placement {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    static void installCallbacks(GLFWwindow* window);

    GLFWwindow* m_window = nullptr;
    Input m_input;
    Placement m_windowedPlacement;
    bool m_cursorCaptured = false;
    bool m_fullscreen = false;
};

} // namespace ghost::platform
