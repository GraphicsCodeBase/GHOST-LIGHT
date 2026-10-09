// Scene-wide settings from the scene file: environment, exposure, player start, camera bookmarks, technique settings.
#pragma once

#include "Assets/AssetRegistry.h"

#include <string>
#include <vector>

#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>

namespace ghost::world {

struct CameraBookmark {
    std::string name;
    glm::vec3 position{0.0f};
    float yawDeg = 0.0f;   // 0 looks down -Z, 90 looks down +X
    float pitchDeg = 0.0f; // positive looks up
};

struct SceneSettings {
    std::string name;
    std::string file;                 // relative to Content/
    std::string environmentHdri;      // relative to Content/; empty = procedural sky
    float environmentIntensity = 1.0f;
    assets::EnvironmentHandle environment = assets::kInvalidEnvironment;
    float exposureEv100 = 14.0f;      // camera exposure; ~14-15 for a sunlit scene with a 100 000 lux sun
    std::string playerMode = "Fly";
    std::string playerStart;          // a bookmark name
    std::vector<CameraBookmark> bookmarks;
    nlohmann::json techniques = nlohmann::json::object(); // raw "techniques" block, read by the technique runtime

    const CameraBookmark* findBookmark(const std::string& bookmarkName) const {
        for (const CameraBookmark& bookmark : bookmarks) {
            if (bookmark.name == bookmarkName) {
                return &bookmark;
            }
        }
        return nullptr;
    }
};

} // namespace ghost::world
