// Smoke test run by `run.bat test`: starts the engine with default settings, runs frames, fails on any logged error.
#include "App/Engine.h"
#include "Core/Log.h"

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

    int exitCode = 1;
    uint64_t frames = 0;
    {
        ghost::app::Engine engine(options);
        if (engine.initialize()) {
            exitCode = engine.run();
            frames = engine.framesRun();
        }
    }

    const int errors = ghost::core::Log::errorCount();
    const bool passed = exitCode == 0 && frames >= kFramesToRun && errors == 0;
    std::printf("\nSmoke test: %llu/%llu frames, %d error(s), engine exit code %d -> %s\n",
                static_cast<unsigned long long>(frames), static_cast<unsigned long long>(kFramesToRun), errors, exitCode,
                passed ? "PASS" : "FAIL");
    return passed ? 0 : 1;
}
