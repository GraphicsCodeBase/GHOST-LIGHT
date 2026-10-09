// Smoke test run by `run.bat test`: starts the engine with default settings, renders frames, resizes the window,
// exercises shader hot reload (break a shader, check the error is reported and the old pipeline survives, fix it),
// loads every scene, checks the GPU scene was filled, and fails on any logged error or any Vulkan validation error.
#include "App/Engine.h"
#include "Assets/AssetRegistry.h"
#include "Core/Log.h"
#include "Core/Paths.h"
#include "DebugTools/FrameCapture.h"
#include "Graphics/GpuScene/GpuScene.h"
#include "Graphics/Passes/ReferencePathTracerPass.h"
#include "Graphics/RayTracing/SceneAccelerationStructures.h"
#include "Graphics/Renderer/Renderer.h"
#include "Graphics/ShaderCompiler/PipelineLibrary.h"
#include "Platform/Window.h"
#include "Sandbox/Player/FlyController.h"
#include "World/Components/MeshRenderer.h"
#include "World/World.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <glm/geometric.hpp>

namespace {

constexpr uint64_t kFramesToRun = 240;

constexpr const char* kGoodShader =
    "// Written by the smoke test to exercise hot reload.\n"
    "[shader(\"compute\")]\n"
    "[numthreads(8, 8, 1)]\n"
    "void main(uint3 id : SV_DispatchThreadID) {}\n";

constexpr const char* kBrokenShader =
    "// Written by the smoke test: deliberately broken on line 4.\n"
    "[shader(\"compute\")]\n"
    "[numthreads(8, 8, 1)]\n"
    "void main(uint3 id : SV_DispatchThreadID) { this is not slang; }\n";

// Writes the file and pushes its timestamp forward so the change is seen even within the file system's time resolution.
void writeShader(const std::filesystem::path& path, const char* text, int secondsAhead) {
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file << text;
    }
    std::error_code ec;
    std::filesystem::last_write_time(path, std::filesystem::file_time_type::clock::now() + std::chrono::seconds(secondsAhead), ec);
}

bool hasError(const ghost::graphics::shader::PipelineLibrary& pipelines, const std::string& name) {
    for (const auto& error : pipelines.errors()) {
        if (error.pipeline == name) {
            return true;
        }
    }
    return false;
}

struct HotReloadCheck {
    enum class Phase { Start, Compiled, WaitingForError, WaitingForRecovery, Done, Failed };
    Phase phase = Phase::Start;
    std::filesystem::path file;
    ghost::graphics::shader::PipelineHandle handle = ghost::graphics::shader::kInvalidPipeline;
    uint64_t reloadsBefore = 0;
    std::string failure;

    void step(ghost::app::Engine& engine, uint64_t frame) {
        auto& pipelines = engine.renderer().pipelines();
        const char* kName = "SmokeTest.HotReload";
        switch (phase) {
        case Phase::Start: {
            file = ghost::core::Paths::build() / "SmokeTest" / "HotReload.slang";
            std::filesystem::create_directories(file.parent_path());
            writeShader(file, kGoodShader, 0);
            handle = pipelines.addCompute({kName, {file, "main", ghost::graphics::shader::ShaderStage::Compute}});
            phase = pipelines.pipeline(handle) != VK_NULL_HANDLE ? Phase::Compiled : fail("initial compile failed");
            break;
        }
        case Phase::Compiled:
            if (frame >= 60) {
                writeShader(file, kBrokenShader, 2);
                phase = Phase::WaitingForError;
            }
            break;
        case Phase::WaitingForError:
            if (hasError(pipelines, kName)) {
                const auto& error = pipelines.errors().front();
                std::printf("hot reload: broken shader reported at %s(%d): %s\n", error.file.c_str(), error.line, error.message.c_str());
                if (error.line != 4 || error.file.find("HotReload.slang") == std::string::npos) {
                    phase = fail("the error location (file, line 4) was not reported");
                    break;
                }
                if (pipelines.pipeline(handle) == VK_NULL_HANDLE) {
                    phase = fail("the last working pipeline was dropped");
                    break;
                }
                reloadsBefore = pipelines.reloadCount();
                writeShader(file, kGoodShader, 4);
                phase = Phase::WaitingForRecovery;
            } else if (frame > 150) {
                phase = fail("the broken shader was never reported");
            }
            break;
        case Phase::WaitingForRecovery:
            if (!hasError(pipelines, kName) && pipelines.reloadCount() > reloadsBefore) {
                std::printf("hot reload: fixed shader reloaded at frame %llu\n", static_cast<unsigned long long>(frame));
                phase = Phase::Done;
            } else if (frame > 230) {
                phase = fail("the fixed shader was never reloaded");
            }
            break;
        default:
            break;
        }
    }

    Phase fail(const char* reason) {
        failure = reason;
        return Phase::Failed;
    }
};

// Scene loading: every shipped scene loads without problems; broken files report file + line and never crash.
struct SceneCheck {
    std::vector<std::string> failures;
    size_t scenesLoaded = 0;

    void fail(std::string reason) { failures.push_back(std::move(reason)); }

    static bool contains(const std::vector<std::string>& messages, const std::string& needle) {
        for (const std::string& message : messages) {
            if (message.find(needle) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    void step(ghost::app::Engine& engine, uint64_t frame) {
        auto& world = engine.world();
        if (frame == 1) {
            const size_t renderers = world.registry().view<ghost::world::MeshRenderer>().size();
            if (renderers < 2 || engine.assets().modelCount() < 2) {
                fail("the default scene did not load its models (" + std::to_string(renderers) + " mesh renderers)");
            }
            const auto& scene = engine.renderer().gpuScene();
            if (!scene.hasGeometry() || scene.models().size() < 2 || scene.instances().empty()) {
                fail("the GPU scene has no geometry after the first frame (" + std::to_string(scene.instances().size()) + " instances)");
            }
        } else if (frame == 90) {
            // Every shipped scene must load cleanly: Engine::loadScene logs problems as errors, which fails the test.
            for (const auto& entry : std::filesystem::directory_iterator(ghost::core::Paths::content() / "Scenes")) {
                const std::string name = ghost::core::Paths::toUtf8(entry.path().filename());
                if (name.ends_with(".scene.json")) {
                    if (engine.loadScene("Scenes/" + name)) {
                        ++scenesLoaded;
                    } else {
                        fail("Scenes/" + name + " did not load");
                    }
                }
            }
        } else if (frame == 95) {
            // Called on the world directly: these problems are expected, so they must not reach the error log.
            const size_t before = world.registry().view<ghost::world::MeshRenderer>().size();
            const auto result = world.loadScene("../Tests/Data/BrokenSyntax.scene.json", engine.assets());
            if (result.loaded || !contains(result.errors, "line 4")) {
                fail("BrokenSyntax.scene.json: expected a parse error at line 4");
            }
            if (world.registry().view<ghost::world::MeshRenderer>().size() != before) {
                fail("a scene that failed to parse replaced the running scene");
            }
        } else if (frame == 96) {
            const auto result = world.loadScene("../Tests/Data/BrokenField.scene.json", engine.assets());
            if (!result.loaded || !contains(result.errors, "BrokenField.scene.json(6): entities[1].transform.position") ||
                !contains(result.errors, "DoesNotExist.gltf") || !contains(result.warnings, "DirectionalLite")) {
                fail("BrokenField.scene.json: expected position/model errors with line numbers and an unknown-component warning");
            }
            for (const std::string& error : result.errors) {
                std::printf("scene check (expected): %s\n", error.c_str());
            }
        } else if (frame == 97) {
            if (!engine.loadScene("Scenes/Sponza.scene.json")) {
                fail("could not return to Sponza");
            }
        }
    }
};

// Player: holding W flies the camera forward (the key is injected the way the window delivers it).
struct PlayerCheck {
    glm::vec3 start{0.0f};
    std::string failure = "never ran";

    void step(ghost::app::Engine& engine, uint64_t frame) {
        ghost::platform::Input& input = engine.window().input();
        const int keyW = static_cast<int>(ghost::platform::Key::W);
        if (frame == 210) {
            start = engine.player().position();
            input.onKey(keyW, true);
        } else if (frame == 220) {
            input.onKey(keyW, false);
            const float moved = glm::dot(engine.player().position() - start, engine.player().forward());
            failure = moved > 0.05f ? "" : "holding W did not move the camera forward";
            std::printf("player: moved %.2f m forward in 10 frames\n", moved);
        }
    }
};

// Ray tracing: acceleration structures exist for the scene, and the reference path tracer accumulates samples in the
// Cornell box while the camera stands still (a restart every frame would mean change detection is broken).
struct PathTracerCheck {
    std::string failure = "never ran";
    std::filesystem::path capture;

    void step(ghost::app::Engine& engine, uint64_t frame) {
        auto& renderer = engine.renderer();
        if (frame == 2) {
            const auto& structures = renderer.accelerationStructures();
            if (structures.blasCount() < 2 || structures.tlasInstanceCount() < 2) {
                failure = "acceleration structures missing (" + std::to_string(structures.blasCount()) + " BLAS, " +
                          std::to_string(structures.tlasInstanceCount()) + " TLAS instances)";
                return;
            }
        } else if (frame == 130) {
            engine.loadScene("Scenes/CornellBox.scene.json");
            renderer.setMode(ghost::graphics::Renderer::Mode::PathTraced);
        } else if (frame == 180) {
            const uint32_t samples = renderer.pathTracer().sampleCount();
            std::printf("path tracer: %u samples after 50 frames\n", samples);
            if (samples < 40) {
                failure = "the path tracer restarted its accumulation (" + std::to_string(samples) + " samples after 50 frames)";
                return;
            }
            capture = ghost::core::Paths::build() / "SmokeTest" / "PathTracedCornellBox.png";
            ghost::debugtools::FrameCapture::requestPng(renderer, capture);
            failure.clear();
        } else if (frame == 190) {
            renderer.setMode(ghost::graphics::Renderer::Mode::Raster);
            engine.loadScene("Scenes/Sponza.scene.json");
        }
    }
};

} // namespace

int main() {
    HotReloadCheck hotReload;
    SceneCheck sceneCheck;
    PlayerCheck playerCheck;
    PathTracerCheck pathTracerCheck;
    std::filesystem::path capturePath;
    size_t passTimings = 0;
    ghost::app::EngineOptions options;
    options.maxFrames = kFramesToRun;
    options.useUserSettings = false;
    options.logFileName = "SmokeTest.log";
    options.windowTitle = "GHOST LIGHT smoke test";
    options.onFrame = [&](ghost::app::Engine& engine, uint64_t frame) {
        // Resizing twice forces swapchain recreation while frames are in flight.
        if (frame == 15) {
            engine.window().setSize(1280, 720);
        } else if (frame == 35) {
            engine.window().setSize(1600, 900);
        }
        hotReload.step(engine, frame);
        sceneCheck.step(engine, frame);
        playerCheck.step(engine, frame);
        pathTracerCheck.step(engine, frame);
        if (frame == 200) {
            // The last rendered image, kept for inspection: Build/SmokeTest/LastFrame.png.
            capturePath = ghost::core::Paths::build() / "SmokeTest" / "LastFrame.png";
            std::error_code ec;
            std::filesystem::remove(capturePath, ec);
            ghost::debugtools::FrameCapture::requestPng(engine.renderer(), capturePath);
        }
        if (frame == kFramesToRun - 1) {
            passTimings = engine.renderer().gpuTimings().size();
        }
    };

    int exitCode = 1;
    uint64_t frames = 0;
    bool validation = false;
    {
        ghost::app::Engine engine(options);
        if (engine.initialize()) {
            validation = engine.validationActive();
            exitCode = engine.run();
            frames = engine.framesRun();
        }
    }

    // "Zero validation errors" only means something if validation actually ran.
#ifdef GHOST_DEBUG
    const bool validationOk = validation;
#else
    const bool validationOk = true;
#endif
    const bool hotReloadOk = hotReload.phase == HotReloadCheck::Phase::Done;
    std::error_code ec;
    const bool captureOk = !capturePath.empty() && std::filesystem::file_size(capturePath, ec) > 1000;
    const bool timingsOk = passTimings >= 5; // TLAS, GBuffer, Lighting, Tonemap, UI
    const bool scenesOk = sceneCheck.failures.empty() && sceneCheck.scenesLoaded >= 2;
    const bool playerOk = playerCheck.failure.empty();
    const bool pathTracerOk = pathTracerCheck.failure.empty();
    if (!pathTracerOk) {
        std::printf("path tracer check FAILED: %s\n", pathTracerCheck.failure.c_str());
    }
    if (!playerOk) {
        std::printf("player check FAILED: %s\n", playerCheck.failure.c_str());
    }
    for (const std::string& failure : sceneCheck.failures) {
        std::printf("scene check FAILED: %s\n", failure.c_str());
    }
    std::printf("scenes: %zu shipped scene(s) loaded cleanly\n", sceneCheck.scenesLoaded);
    const int errors = ghost::core::Log::errorCount();
    const bool passed = exitCode == 0 && frames >= kFramesToRun && errors == 0 && validationOk && hotReloadOk && captureOk && timingsOk &&
                        scenesOk && playerOk && pathTracerOk;
    std::printf("\nSmoke test: %llu/%llu frames, %d error(s), validation %s, hot reload %s, capture %s, GPU timers %zu passes, "
                "engine exit code %d -> %s\n",
                static_cast<unsigned long long>(frames), static_cast<unsigned long long>(kFramesToRun), errors,
                validation ? "on" : "OFF", hotReloadOk ? "ok" : ("FAILED: " + hotReload.failure).c_str(), captureOk ? "ok" : "MISSING",
                passTimings, exitCode, passed ? "PASS" : "FAIL");
    if (!validationOk) {
        std::printf("Validation layers did not load in a Debug build: run run.bat so it installs them into .tools/.\n");
    }
    return passed ? 0 : 1;
}
