// SpotLight component: a cone light at the entity's position, pointing along the entity's -Z axis.
#pragma once

#include <glm/vec3.hpp>

namespace ghost::world {

struct SpotLight {
    glm::vec3 color{1.0f};
    float intensity = 500.0f;     // candela on the axis
    float radius = 0.05f;         // meters
    float innerConeDeg = 20.0f;   // full intensity inside
    float outerConeDeg = 30.0f;   // zero outside
    float range = 0.0f;           // 0 = unlimited
};

} // namespace ghost::world
