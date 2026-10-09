// Engine startup (paths, log, settings, window) and the main loop.
#include "App/Engine.h"

#include "Assets/AssetRegistry.h"
#include "Core/Log.h"
#include "Core/Paths.h"
#include "DebugTools/FrameCapture.h"
#include "Graphics/GpuScene/GpuScene.h"
#include "Graphics/Passes/ReferencePathTracerPass.h"
#include "Graphics/Renderer/Renderer.h"
#include "Graphics/ShaderCompiler/PipelineLibrary.h"
#include "Platform/Window.h"
#include "Sandbox/Player/FlyController.h"
#include "UI/ImGuiLayer.h"
#include "UI/PerformanceOverlay.h"
#include "UI/SceneErrorOverlay.h"
#include "UI/ShaderErrorOverlay.h"
#include "World/Systems/GpuSceneExtractionSystem.h"
#include "World/World.h"

#include <cmath>
#include <cstdio>
#include <format>
#include <utility>

#include <windows.h>

namespace ghost::app {

namespace {

constexpr const char* kDefaultScene = "Scenes/Sponza.scene.json";

#ifdef GHOST_DEBUG
constexpr const char* kBuildConfig = "Debug";
#else
constexpr const char* kBuildConfig = "Release";
#endif

} // namespace

Engine::Engine(EngineOptions options) : m_options(std::move(options)) {}

Engine::~Engine() {
    if (m_initialized) {
        core::Log::info("Shutting down after {} frames", m_framesRun);
    }
    if (m_renderer) {
        m_renderer->waitIdle(); // ImGui's Vulkan objects may still be used by frames in flight
    }
    m_ui.reset();
    m_renderer.reset(); // the Vulkan surface must go before the window it was created from
    m_window.reset();
    core::Log::closeFile();
}

bool Engine::initialize() {
    SetConsoleOutputCP(CP_UTF8);

    if (!core::Paths::initialize()) {
        std::fprintf(stderr,
                     "GhostLight.exe must run from inside its GHOST LIGHT folder (it looks for run.bat in a parent folder).\n"
                     "Start it with run.bat.\n");
        return false;
    }
    core::Log::openFile(core::Paths::build() / m_options.logFileName);
    core::Log::info("GHOST LIGHT ({}) starting in {}", kBuildConfig, core::Paths::toUtf8(core::Paths::root()));

    if (m_options.useUserSettings) {
        m_settings.load();
    }

    platform::Window::Desc desc;
    desc.title = m_options.windowTitle;
    desc.width = m_settings.windowWidth;
    desc.height = m_settings.windowHeight;
    desc.maximized = m_settings.windowMaximized;
    m_window = std::make_unique<platform::Window>(desc);
    if (!m_window->isValid()) {
        return false;
    }

    graphics::Renderer::Desc rendererDesc;
    rendererDesc.validation = m_options.validation;
    rendererDesc.vsync = m_settings.vsync;
    m_renderer = std::make_unique<graphics::Renderer>();
    if (!m_renderer->initialize(*m_window, rendererDesc)) {
        return false;
    }
    m_ui = std::make_unique<ui::ImGuiLayer>();
    if (!m_ui->initialize(*m_window, *m_renderer, m_options.useUserSettings)) {
        return false;
    }

    m_assets = std::make_unique<assets::AssetRegistry>();
    m_world = std::make_unique<world::World>();
    m_extraction = std::make_unique<world::GpuSceneExtractionSystem>();
    m_player = std::make_unique<sandbox::FlyController>();
    std::string scene = m_options.scene;
    if (scene.empty()) {
        scene = (m_options.useUserSettings && !m_settings.lastScene.empty()) ? m_settings.lastScene : kDefaultScene;
    }
    if (!loadScene(scene) && scene != kDefaultScene) {
        core::Log::warning("Falling back to the default scene {}", kDefaultScene);
        loadScene(kDefaultScene);
    }

    m_initialized = true;
    return true;
}

int Engine::run() {
    if (!m_initialized) {
        return 1;
    }

    while (!m_window->shouldClose()) {
        m_window->pollEvents();
        if (m_window->isMinimized()) {
            m_window->waitEvents();
            continue;
        }

        m_timer.tick();
        if (m_options.onFrame) {
            m_options.onFrame(*this, m_framesRun);
        }
        handleGlobalShortcuts();
        updateWindowTitle();

        world::SceneLoadResult reload;
        if (m_world->reloadIfChanged(*m_assets, reload)) {
            reportSceneResult(reload, m_world->currentScene());
        }
        m_world->update();
        m_player->update(*m_window, static_cast<float>(m_timer.deltaSeconds()), m_settings.cameraSpeed);
        updateGpuScene();

        drawUi();
        m_renderer->renderFrame([this](VkCommandBuffer cmd) { m_ui->record(cmd); });

        ++m_framesRun;
        if (m_options.maxFrames != 0 && m_framesRun >= m_options.maxFrames) {
            break;
        }
    }

    saveUserSettings();
    return 0;
}

void Engine::drawUi() {
    m_ui->beginFrame();
    ui::PerformanceOverlay::Stats stats;
    stats.fps = m_timer.smoothedFps();
    stats.gpuName = m_renderer->gpuName();
    stats.validation = m_renderer->validationActive();
    const glm::ivec2 size = m_window->framebufferSize();
    stats.width = size.x;
    stats.height = size.y;
    stats.mode = m_renderer->mode() == graphics::Renderer::Mode::Raster
                     ? "Raster  (F5: path tracer)"
                     : std::format("Path traced, {} samples  (F5: raster)", m_renderer->pathTracer().sampleCount());
    stats.gpuMilliseconds = m_renderer->gpuFrameMilliseconds();
    for (const auto& timing : m_renderer->gpuTimings()) {
        stats.passTimings.emplace_back(timing.name, timing.milliseconds);
    }
    ui::PerformanceOverlay::draw(stats);
    ui::ShaderErrorOverlay::draw(m_renderer->pipelines().errors());
    ui::SceneErrorOverlay::draw(m_sceneErrors, m_sceneWarnings);
    m_ui->endFrame();
}

bool Engine::loadScene(const std::string& contentPath) {
    const world::SceneLoadResult result = m_world->loadScene(contentPath, *m_assets);
    reportSceneResult(result, contentPath);
    if (result.loaded) {
        placePlayerAtStart();
        if (m_options.useUserSettings) {
            m_settings.lastScene = contentPath;
        }
    }
    return result.loaded;
}

void Engine::placePlayerAtStart() {
    const world::SceneSettings& settings = m_world->settings();
    const world::CameraBookmark* start = settings.findBookmark(settings.playerStart);
    if (!start && !settings.bookmarks.empty()) {
        start = &settings.bookmarks.front();
    }
    if (start) {
        m_player->teleport(start->position, start->yawDeg, start->pitchDeg);
    } else {
        m_player->teleport({0.0f, 1.7f, 5.0f}, 0.0f, 0.0f);
    }
    m_resetCameraHistory = true;
}

void Engine::updateGpuScene() {
    graphics::scene::GpuScene& scene = m_renderer->gpuScene();
    m_extraction->update(m_world->registry(), m_world->settings(), *m_assets, scene);
    scene.setCamera({m_player->position(), m_player->forward(), m_player->fovYDeg, m_player->nearPlane}, m_resetCameraHistory);
    m_resetCameraHistory = false;
    // Physical camera exposure from EV100 (ISO 100, saturation-based sensitivity): 1 / (1.2 * 2^EV100).
    scene.setExposure(1.0f / (1.2f * std::exp2(m_world->settings().exposureEv100)));
}

void Engine::reportSceneResult(const world::SceneLoadResult& result, const std::string& contentPath) {
    for (const std::string& error : result.errors) {
        core::Log::error("{}", error);
    }
    for (const std::string& warning : result.warnings) {
        core::Log::warning("{}", warning);
    }
    if (result.loaded) {
        core::Log::info("Scene {} loaded: {} entities, {} error(s), {} warning(s)", contentPath, result.entityCount, result.errors.size(),
                        result.warnings.size());
    } else {
        core::Log::error("Scene {} was not loaded; the previous scene stays", contentPath);
    }
    m_sceneErrors = result.errors;
    m_sceneWarnings = result.warnings;
}

bool Engine::validationActive() const {
    return m_renderer && m_renderer->validationActive();
}

void Engine::handleGlobalShortcuts() {
    const platform::Input& input = m_window->input();
    if (input.wasPressed(platform::Key::F11)) {
        m_window->toggleFullscreen();
    }
    if (input.wasPressed(platform::Key::F5)) {
        const bool raster = m_renderer->mode() == graphics::Renderer::Mode::Raster;
        m_renderer->setMode(raster ? graphics::Renderer::Mode::PathTraced : graphics::Renderer::Mode::Raster);
    }
    if (input.wasPressed(platform::Key::F12)) {
        debugtools::FrameCapture::requestPng(*m_renderer, debugtools::FrameCapture::defaultPath());
    }
}

void Engine::updateWindowTitle() {
    m_titleRefreshSeconds -= m_timer.deltaSeconds();
    if (m_titleRefreshSeconds > 0.0) {
        return;
    }
    m_titleRefreshSeconds = 0.5;
    const double fps = m_timer.smoothedFps();
    m_window->setTitle(std::format("{}  |  {:.0f} fps  {:.2f} ms", m_options.windowTitle, fps, fps > 0.0 ? 1000.0 / fps : 0.0));
}

void Engine::saveUserSettings() {
    if (!m_options.useUserSettings || !m_window) {
        return;
    }
    m_settings.windowMaximized = m_window->isMaximized();
    if (!m_settings.windowMaximized && !m_window->isFullscreen()) {
        const glm::ivec2 size = m_window->windowSize();
        m_settings.windowWidth = size.x;
        m_settings.windowHeight = size.y;
    }
    m_settings.save();
}

} // namespace ghost::app
