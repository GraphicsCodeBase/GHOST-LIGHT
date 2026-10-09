// The engine: owns the window, user settings and (in later steps) renderer and world, and runs the frame loop.
#pragma once

#include "App/EngineOptions.h"
#include "Core/FrameTimer.h"
#include "Core/UserSettings.h"

#include <cstdint>
#include <memory>

namespace ghost::platform {
class Window;
}
namespace ghost::graphics {
class Renderer;
}
namespace ghost::ui {
class ImGuiLayer;
}

namespace ghost::app {

class Engine {
public:
    explicit Engine(EngineOptions options);
    ~Engine();
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // Prepares everything needed to run. Returns false after logging a clear reason.
    bool initialize();
    // Runs frames until the window closes or maxFrames is reached. Returns the process exit code.
    int run();

    uint64_t framesRun() const { return m_framesRun; }
    platform::Window& window() { return *m_window; }
    graphics::Renderer& renderer() { return *m_renderer; }
    // True when the Vulkan validation layers are loaded and reporting into the log.
    bool validationActive() const;

private:
    void handleGlobalShortcuts();
    void updateWindowTitle();
    void drawUi();
    void saveUserSettings();

    EngineOptions m_options;
    core::UserSettings m_settings;
    core::FrameTimer m_timer;
    std::unique_ptr<platform::Window> m_window;
    std::unique_ptr<graphics::Renderer> m_renderer;
    std::unique_ptr<ui::ImGuiLayer> m_ui;
    uint64_t m_framesRun = 0;
    double m_titleRefreshSeconds = 0.0;
    bool m_initialized = false;
};

} // namespace ghost::app
