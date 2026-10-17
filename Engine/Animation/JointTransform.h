// A node's transform relative to its parent, kept as translation / rotation / scale so clips can blend each part.
#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace ghost::animation {

struct JointTransform {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; // glm stores w first; glTF files store x, y, z, w
    glm::vec3 scale{1.0f};
};

} // namespace ghost::animation
