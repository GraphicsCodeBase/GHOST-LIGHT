// The ECS world: the EnTT registry of the current scene, its scene-wide settings, scene loading and hot reload.
#pragma once

#include "World/SceneSettings.h"
#include "World/Serialization/SceneLoader.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <entt/entity/registry.hpp>

namespace ghost::world {

class World {
public:
    entt::registry& registry() { return m_registry; }
    const entt::registry& registry() const { return m_registry; }
    const SceneSettings& settings() const { return m_settings; }
    SceneSettings& settings() { return m_settings; }

    // Loads a scene (path relative to Content/). If the file can't be read or parsed the current scene stays and the
    // result says why; entity-level problems are reported but the rest of the scene still loads.
    SceneLoadResult loadScene(const std::string& contentPath, assets::AssetRegistry& assets);
    // Reloads the current scene when it or one of its prefab files changed on disk (checked at most twice a second).
    bool reloadIfChanged(assets::AssetRegistry& assets, SceneLoadResult& result);

    // Runs the per-frame systems (transforms for now; lights, physics, sandbox later).
    void update();

    const std::string& currentScene() const { return m_settings.file; }
    // Increments with every successful (re)load: the GPU scene uses it to rebuild.
    uint64_t sceneRevision() const { return m_sceneRevision; }

private:
    struct WatchedFile {
        std::filesystem::path path;
        std::filesystem::file_time_type time;
    };

    entt::registry m_registry;
    SceneSettings m_settings;
    std::vector<WatchedFile> m_watched;
    std::chrono::steady_clock::time_point m_lastCheck{};
    uint64_t m_sceneRevision = 0;
};

} // namespace ghost::world
