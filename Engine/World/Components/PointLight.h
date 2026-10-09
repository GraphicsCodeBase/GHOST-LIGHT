// PointLight component: a small spherical light at the entity's position. Intensity in candela (lumens per steradian).
#pragma once

#include <glm/vec3.hpp>

namespace ghost::world {

struct PointLight {
    glm::vec3 color{1.0f};
    float intensity = 100.0f; // candela
    float radius = 0.05f;     // meters (soft shadows, area sampling)
    float range = 0.0f;       // 0 = unlimited
};

} // namespace ghost::world
