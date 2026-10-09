// PrefabInstance component: the prefab file an entity was created from (relative to Content/), kept for saving scenes.
#pragma once

#include <string>

namespace ghost::world {

struct PrefabInstance {
    std::string path;
};

} // namespace ghost::world
