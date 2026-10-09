// Smoke test run by `run.bat test`: starts the engine with default settings, renders frames, resizes the window,
// exercises shader hot reload (break a shader, check the error is reported and the old pipeline survives, fix it),
// and fails on any logged error or any Vulkan validation error.
#include "App/Engine.h"
#include "Core/Log.h"
#include "Core/Paths.h"
#include "Graphics/Renderer/Renderer.h"
#include "Graphics/ShaderCompiler/PipelineLibrary.h"
#include "Platform/Window.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

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

} // namespace

int main() {
    HotReloadCheck hotReload;
    ghost::app::EngineOptions options;
    options.maxFrames = kFramesToRun;
    options.useUserSettings = false;
    options.logFileName = "SmokeTest.log";
    options.windowTitle = "GHOST LIGHT smoke test";
    options.onFrame = [&hotReload](ghost::app::Engine& engine, uint64_t frame) {
        // Resizing twice forces swapchain recreation while frames are in flight.
        if (frame == 15) {
            engine.window().setSize(1280, 720);
        } else if (frame == 35) {
            engine.window().setSize(1600, 900);
        }
        hotReload.step(engine, frame);
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
    const int errors = ghost::core::Log::errorCount();
    const bool passed = exitCode == 0 && frames >= kFramesToRun && errors == 0 && validationOk && hotReloadOk;
    std::printf("\nSmoke test: %llu/%llu frames, %d error(s), validation %s, hot reload %s, engine exit code %d -> %s\n",
                static_cast<unsigned long long>(frames), static_cast<unsigned long long>(kFramesToRun), errors,
                validation ? "on" : "OFF", hotReloadOk ? "ok" : ("FAILED: " + hotReload.failure).c_str(), exitCode,
                passed ? "PASS" : "FAIL");
    if (!validationOk) {
        std::printf("Validation layers did not load in a Debug build: run run.bat so it installs them into .tools/.\n");
    }
    return passed ? 0 : 1;
}
