// Startup options for the engine, filled from the command line (GhostLight.exe) or by the smoke test.
#pragma once

#include <cstdint>
#include <string>

namespace ghost::app {

struct EngineOptions {
    uint64_t maxFrames = 0;          // 0 = run until the window is closed
    bool useUserSettings = true;     // false: ignore and never write User/settings.json (smoke test)
    std::string scene;               // relative to Content/; empty = last scene or the default scene
    std::string logFileName = "GhostLight.log"; // written to Build/
    std::string windowTitle = "GHOST LIGHT";
};

} // namespace ghost::app
