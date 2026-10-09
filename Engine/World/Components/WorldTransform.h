// WorldTransform component: the entity's final local-to-world matrix this frame and last frame (for motion vectors).
// Written only by TransformSystem.
#pragma once

#include <glm/mat4x4.hpp>

namespace ghost::world {

struct WorldTransform {
    glm::mat4 matrix{1.0f};
    glm::mat4 previous{1.0f}; // last frame's matrix; equals `matrix` on the first frame an entity exists
};

} // namespace ghost::world
