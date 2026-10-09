// MeshRenderer component: which model an entity draws ("Assets/Models/X/X.gltf" or "procedural:Name").
#pragma once

#include "Assets/AssetRegistry.h"

#include <string>

namespace ghost::world {

struct MeshRenderer {
    std::string model;                         // as written in the scene/prefab file
    assets::ModelHandle handle = assets::kInvalidModel;
    bool visible = true;
};

} // namespace ghost::world
