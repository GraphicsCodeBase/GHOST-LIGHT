// The engine: owns the window, user settings, renderer, assets, world and player, and runs the frame loop.
#pragma once

#include "App/EngineOptions.h"
#include "Core/FrameTimer.h"
#include "Core/UserSettings.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ghost::platform {
class Window;
}
namespace ghost::graphics {
class Renderer;
}
namespace ghost::ui {
class ImGuiLayer;
}
namespace ghost::assets {
class AssetRegistry;
}
namespace ghost::world {
class World;
class GpuSceneExtractionSystem;
struct SceneLoadResult;
} // namespace ghost::world
namespace ghost::sandbox {
class FlyController;
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
    world::World& world() { return *m_world; }
    assets::AssetRegistry& assets() { return *m_assets; }
    sandbox::FlyController& player() { return *m_player; }

    // Loads a scene (relative to Content/), logs its problems and shows them on screen. Returns false when the file
    // could not be read or parsed (the previous scene stays).
    bool loadScene(const std::string& contentPath);
    // True when the Vulkan validation layers are loaded and reporting into the log.
    bool validationActive() const;

private:
    void handleGlobalShortcuts();
    void updateWindowTitle();
    void drawUi();
    void reportSceneResult(const world::SceneLoadResult& result, const std::string& contentPath);
    void placePlayerAtStart();
    void updateGpuScene();
    void saveUserSettings();

    EngineOptions m_options;
    core::UserSettings m_settings;
    core::FrameTimer m_timer;
    std::unique_ptr<platform::Window> m_window;
    std::unique_ptr<graphics::Renderer> m_renderer;
    std::unique_ptr<ui::ImGuiLayer> m_ui;
    std::unique_ptr<assets::AssetRegistry> m_assets;
    std::unique_ptr<world::World> m_world;
    std::unique_ptr<world::GpuSceneExtractionSystem> m_extraction;
    std::unique_ptr<sandbox::FlyController> m_player;
    bool m_resetCameraHistory = true; // no motion vectors across a teleport
    std::vector<std::string> m_sceneErrors;   // shown by SceneErrorOverlay until dismissed
    std::vector<std::string> m_sceneWarnings;
    uint64_t m_framesRun = 0;
    double m_titleRefreshSeconds = 0.0;
    bool m_initialized = false;
};

} // namespace ghost::app
