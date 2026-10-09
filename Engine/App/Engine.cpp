// Engine startup (paths, log, settings, window) and the main loop.
#include "App/Engine.h"

#include "Core/Log.h"
#include "Core/Paths.h"
#include "Graphics/Renderer/Renderer.h"
#include "Graphics/ShaderCompiler/PipelineLibrary.h"
#include "Platform/Window.h"
#include "UI/ImGuiLayer.h"
#include "UI/PerformanceOverlay.h"
#include "UI/ShaderErrorOverlay.h"

#include <cstdio>
#include <format>
#include <utility>

#include <windows.h>

namespace ghost::app {

namespace {

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
        drawUi();
        m_renderer->renderFrame(m_timer.elapsedSeconds(), [this](VkCommandBuffer cmd) { m_ui->record(cmd); });

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
    ui::PerformanceOverlay::draw(stats);
    ui::ShaderErrorOverlay::draw(m_renderer->pipelines().errors());
    m_ui->endFrame();
}

bool Engine::validationActive() const {
    return m_renderer && m_renderer->validationActive();
}

void Engine::handleGlobalShortcuts() {
    const platform::Input& input = m_window->input();
    if (input.wasPressed(platform::Key::F11)) {
        m_window->toggleFullscreen();
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
