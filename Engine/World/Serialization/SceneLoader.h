// Reads a *.scene.json (and the prefabs it uses) into an EnTT registry. Never throws: problems are collected as
// "file(line): field: message", a broken entity is skipped, and an unreadable file leaves nothing half-loaded.
#pragma once

#include "Assets/AssetRegistry.h"
#include "World/SceneSettings.h"

#include <filesystem>
#include <string>
#include <vector>

#include <entt/entity/fwd.hpp>

namespace ghost::world {

struct SceneLoadResult {
    bool loaded = false;                        // false: the file could not be read or parsed (nothing was loaded)
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<std::filesystem::path> files;   // the scene and every prefab read (watched for hot reload)
    size_t entityCount = 0;
};

class SceneLoader {
public:
    // `contentPath` is relative to Content/, e.g. "Scenes/Sponza.scene.json". Fills an empty registry and settings.
    static SceneLoadResult load(const std::string& contentPath, entt::registry& registry, SceneSettings& settings,
                                assets::AssetRegistry& assets);
};

} // namespace ghost::world
