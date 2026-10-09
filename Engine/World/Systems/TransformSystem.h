// TransformSystem: turns each entity's Transform (relative to its parent) into a WorldTransform, keeping last frame's
// matrix for motion vectors.
#pragma once

#include <entt/entity/fwd.hpp>

namespace ghost::world {

class TransformSystem {
public:
    static void update(entt::registry& registry);
};

} // namespace ghost::world
