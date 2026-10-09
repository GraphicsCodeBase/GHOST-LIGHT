// GLFW window creation, event callbacks into Input, and window-state helpers (fullscreen, cursor capture).
#include "Platform/Window.h"

#include "Core/Log.h"

#include <algorithm>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace ghost::platform {

namespace {

int g_windowCount = 0;

void onGlfwError(int code, const char* description) {
    core::Log::error("GLFW error {}: {}", code, description);
}

Window* windowFrom(GLFWwindow* handle) {
    return static_cast<Window*>(glfwGetWindowUserPointer(handle));
}

// The monitor that contains most of the window, so fullscreen opens where the window is.
GLFWmonitor* monitorUnder(GLFWwindow* window) {
    int wx = 0;
    int wy = 0;
    int ww = 0;
    int wh = 0;
    glfwGetWindowPos(window, &wx, &wy);
    glfwGetWindowSize(window, &ww, &wh);

    int count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    GLFWmonitor* best = glfwGetPrimaryMonitor();
    int bestArea = 0;
    for (int i = 0; i < count; ++i) {
        int mx = 0;
        int my = 0;
        glfwGetMonitorPos(monitors[i], &mx, &my);
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
        const int overlapX = std::max(0, std::min(wx + ww, mx + mode->width) - std::max(wx, mx));
        const int overlapY = std::max(0, std::min(wy + wh, my + mode->height) - std::max(wy, my));
        if (overlapX * overlapY > bestArea) {
            bestArea = overlapX * overlapY;
            best = monitors[i];
        }
    }
    return best;
}

} // namespace

Window::Window(const Desc& desc) {
    if (g_windowCount == 0) {
        glfwSetErrorCallback(onGlfwError);
        if (!glfwInit()) {
            core::Log::error("Could not initialize GLFW (no display available?)");
            return;
        }
    }
    ++g_windowCount;

    // Fit the requested size into the primary monitor's work area; maximize when it doesn't fit.
    int areaX = 0;
    int areaY = 0;
    int areaWidth = desc.width;
    int areaHeight = desc.height;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &areaX, &areaY, &areaWidth, &areaHeight);
    constexpr int kTitleBarAllowance = 40;
    const bool fits = desc.width <= areaWidth && desc.height + kTitleBarAllowance <= areaHeight;
    const int width = fits ? desc.width : std::min(desc.width, areaWidth);
    const int height = fits ? desc.height : std::min(desc.height, areaHeight - kTitleBarAllowance);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_MAXIMIZED, (desc.maximized || !fits) ? GLFW_TRUE : GLFW_FALSE);
    m_window = glfwCreateWindow(width, height, desc.title.c_str(), nullptr, nullptr);
    if (!m_window) {
        core::Log::error("Could not create the window ({}x{})", width, height);
        return;
    }

    if (fits && !desc.maximized) {
        glfwSetWindowPos(m_window, areaX + (areaWidth - width) / 2, areaY + (areaHeight - height) / 2);
    }
    glfwSetWindowUserPointer(m_window, this);
    installCallbacks(m_window);
    glfwShowWindow(m_window);
    glfwFocusWindow(m_window);
}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    if (g_windowCount > 0 && --g_windowCount == 0) {
        glfwTerminate();
    }
}

void Window::installCallbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, [](GLFWwindow* handle, int key, int /*scancode*/, int action, int /*mods*/) {
        if (action != GLFW_REPEAT) {
            windowFrom(handle)->m_input.onKey(key, action == GLFW_PRESS);
        }
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow* handle, int button, int action, int /*mods*/) {
        windowFrom(handle)->m_input.onMouseButton(button, action == GLFW_PRESS);
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow* handle, double x, double y) {
        windowFrom(handle)->m_input.onCursorMoved(x, y);
    });
    glfwSetScrollCallback(window, [](GLFWwindow* handle, double /*x*/, double y) {
        windowFrom(handle)->m_input.onScroll(y);
    });
}

void Window::pollEvents() {
    m_input.beginFrame();
    glfwPollEvents();
}

void Window::waitEvents() {
    glfwWaitEventsTimeout(0.1);
}

bool Window::shouldClose() const {
    return !m_window || glfwWindowShouldClose(m_window);
}

void Window::requestClose() {
    if (m_window) {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
}

glm::ivec2 Window::framebufferSize() const {
    glm::ivec2 size(0);
    if (m_window) {
        glfwGetFramebufferSize(m_window, &size.x, &size.y);
    }
    return size;
}

glm::ivec2 Window::windowSize() const {
    glm::ivec2 size(0);
    if (m_window) {
        glfwGetWindowSize(m_window, &size.x, &size.y);
    }
    return size;
}

bool Window::isMinimized() const {
    return m_window && glfwGetWindowAttrib(m_window, GLFW_ICONIFIED);
}

bool Window::isMaximized() const {
    return m_window && glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED);
}

void Window::setSize(int width, int height) {
    if (!m_window) {
        return;
    }
    if (isMaximized()) {
        glfwRestoreWindow(m_window);
    }
    glfwSetWindowSize(m_window, width, height);
}

void* Window::nativeHandle() const {
    return m_window ? static_cast<void*>(glfwGetWin32Window(m_window)) : nullptr;
}

void Window::setTitle(const std::string& title) {
    if (m_window) {
        glfwSetWindowTitle(m_window, title.c_str());
    }
}

void Window::setCursorCaptured(bool captured) {
    if (!m_window || captured == m_cursorCaptured) {
        return;
    }
    m_cursorCaptured = captured;
    glfwSetInputMode(m_window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (captured && glfwRawMouseMotionSupported()) {
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
}

void Window::toggleFullscreen() {
    if (!m_window) {
        return;
    }
    if (!m_fullscreen) {
        glfwGetWindowPos(m_window, &m_windowedPlacement.x, &m_windowedPlacement.y);
        glfwGetWindowSize(m_window, &m_windowedPlacement.width, &m_windowedPlacement.height);
        GLFWmonitor* monitor = monitorUnder(m_window);
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        m_fullscreen = true;
    } else {
        const Placement& p = m_windowedPlacement;
        glfwSetWindowMonitor(m_window, nullptr, p.x, p.y, p.width, p.height, GLFW_DONT_CARE);
        m_fullscreen = false;
    }
}

} // namespace ghost::platform
