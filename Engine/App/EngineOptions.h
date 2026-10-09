// Startup options for the engine, filled from the command line (GhostLight.exe) or by the smoke test.
#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace ghost::app {

class Engine;

struct EngineOptions {
    // Called at the start of every frame with the frame index; the smoke test uses it to resize, switch scenes, etc.
    std::function<void(Engine&, uint64_t)> onFrame;
    uint64_t maxFrames = 0;          // 0 = run until the window is closed
    bool useUserSettings = true;     // false: ignore and never write User/settings.json (smoke test)
    std::string scene;               // relative to Content/; empty = last scene or the default scene
    std::string logFileName = "GhostLight.log"; // written to Build/
    std::string windowTitle = "GHOST LIGHT";
#ifdef GHOST_DEBUG
    bool validation = true;          // Vulkan validation layers (Debug default: on)
#else
    bool validation = false;
#endif
};

} // namespace ghost::app
