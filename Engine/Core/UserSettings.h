// Machine-specific settings (window size, last scene, camera speed) kept in the gitignored User/settings.json.
#pragma once

#include <string>

namespace ghost::core {

class UserSettings {
public:
    int windowWidth = 1920;
    int windowHeight = 1080;
    bool windowMaximized = false;
    std::string lastScene;    // relative to Content/, e.g. "Scenes/Sponza.scene.json"; empty = default scene
    float cameraSpeed = 5.0f; // meters per second
    bool vsync = true;        // off = uncapped frame rate (better for timing measurements)

    // Reads User/settings.json, creating it with defaults when missing. A corrupt file is kept as
    // settings.json.bad and replaced with defaults, so a bad edit can never stop the engine from starting.
    void load();
    void save() const;
};

} // namespace ghost::core
