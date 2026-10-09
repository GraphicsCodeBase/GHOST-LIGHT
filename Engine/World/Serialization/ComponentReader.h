// Turns one JSON component block ("MeshRenderer": {...}, "DirectionalLight": {...}, ...) into an EnTT component.
// The list of what a scene or prefab may contain lives here.
#pragma once

#include "Assets/AssetRegistry.h"

#include <string>
#include <vector>

#include <entt/entity/fwd.hpp>

namespace ghost::core {
class JsonReader;
}

namespace ghost::world {

struct Transform;

class ComponentReader {
public:
    // Adds or replaces the component on the entity. Unknown component names produce a warning (typos are caught).
    static void apply(const std::string& name, const core::JsonReader& data, entt::registry& registry, entt::entity entity,
                      assets::AssetRegistry& assets, std::vector<std::string>& assetWarnings);
    // "transform": position, rotationEuler (degrees) or rotation (quaternion xyzw), scale (number or [x, y, z]).
    static void readTransform(const core::JsonReader& data, Transform& transform);
};

} // namespace ghost::world
