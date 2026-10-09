// Transform component: position, rotation and scale relative to the parent entity (or the world when there is none).
#pragma once

#include <entt/entity/entity.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace ghost::world {

struct Transform {
    glm::vec3 position{0.0f};          // meters
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    entt::entity parent = entt::null;
};

} // namespace ghost::world
