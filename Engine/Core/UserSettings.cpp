// User/settings.json reading and writing. Unknown keys are ignored and missing keys keep their defaults.
#include "Core/UserSettings.h"

#include "Core/JsonFile.h"
#include "Core/Log.h"
#include "Core/Paths.h"

#include <algorithm>
#include <system_error>

namespace ghost::core {

namespace {

constexpr int kSettingsVersion = 1;

std::filesystem::path settingsPath() {
    return Paths::user() / "settings.json";
}

} // namespace

void UserSettings::load() {
    const std::filesystem::path path = settingsPath();
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        Log::info("Creating {} with default settings", Paths::display(path));
        save();
        return;
    }

    const JsonFile::LoadResult file = JsonFile::load(path);
    bool valid = file.ok && file.value.is_object();
    if (valid) {
        try {
            const nlohmann::json& json = file.value;
            windowWidth = json.value("windowWidth", windowWidth);
            windowHeight = json.value("windowHeight", windowHeight);
            windowMaximized = json.value("windowMaximized", windowMaximized);
            lastScene = json.value("lastScene", lastScene);
            cameraSpeed = json.value("cameraSpeed", cameraSpeed);
            vsync = json.value("vsync", vsync);
        } catch (const nlohmann::json::exception& e) {
            Log::warning("{}: {}", Paths::display(path), e.what());
            valid = false;
        }
    }

    if (!valid) {
        std::filesystem::path backup = path;
        backup += ".bad";
        std::filesystem::rename(path, backup, ec);
        Log::warning("{} is invalid ({}). Kept it as {} and recreated it with defaults.", Paths::display(path),
                     file.ok ? "unexpected value types" : file.error, Paths::display(backup));
        *this = UserSettings{};
        save();
        return;
    }

    windowWidth = std::clamp(windowWidth, 640, 16384);
    windowHeight = std::clamp(windowHeight, 360, 16384);
    cameraSpeed = std::clamp(cameraSpeed, 0.01f, 10000.0f);
}

void UserSettings::save() const {
    nlohmann::json json;
    json["version"] = kSettingsVersion;
    json["windowWidth"] = windowWidth;
    json["windowHeight"] = windowHeight;
    json["windowMaximized"] = windowMaximized;
    json["lastScene"] = lastScene;
    json["cameraSpeed"] = cameraSpeed;
    json["vsync"] = vsync;

    std::string error;
    if (!JsonFile::save(settingsPath(), json, error)) {
        Log::warning("Could not save user settings: {}", error);
    }
}

} // namespace ghost::core
