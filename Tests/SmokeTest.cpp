// Smoke test run by `run.bat test`: starts the engine with default settings, runs frames, fails on any logged error.
#include "App/Engine.h"
#include "Core/Log.h"
#include "Platform/Window.h"

#include <cstdio>

namespace {

constexpr uint64_t kFramesToRun = 60;

} // namespace

int main() {
    ghost::app::EngineOptions options;
    options.maxFrames = kFramesToRun;
    options.useUserSettings = false;
    options.logFileName = "SmokeTest.log";
    options.windowTitle = "GHOST LIGHT smoke test";
    // Resizing twice forces swapchain recreation while frames are in flight.
    options.onFrame = [](ghost::app::Engine& engine, uint64_t frame) {
        if (frame == 15) {
            engine.window().setSize(1280, 720);
        } else if (frame == 35) {
            engine.window().setSize(1600, 900);
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
    const int errors = ghost::core::Log::errorCount();
    const bool passed = exitCode == 0 && frames >= kFramesToRun && errors == 0 && validationOk;
    std::printf("\nSmoke test: %llu/%llu frames, %d error(s), validation %s, engine exit code %d -> %s\n",
                static_cast<unsigned long long>(frames), static_cast<unsigned long long>(kFramesToRun), errors,
                validation ? "on" : "OFF", exitCode, passed ? "PASS" : "FAIL");
    if (!validationOk) {
        std::printf("Validation layers did not load in a Debug build: run run.bat so it installs them into .tools/.\n");
    }
    return passed ? 0 : 1;
}
