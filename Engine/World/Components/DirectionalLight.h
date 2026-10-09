// DirectionalLight component: a sun. Physical units: illuminance in lux on a surface facing the light.
#pragma once

#include <glm/vec3.hpp>

namespace ghost::world {

struct DirectionalLight {
    glm::vec3 direction{-0.3f, -1.0f, 0.2f}; // direction the light travels (normalized on load)
    float illuminance = 100000.0f;            // lux; clear-sky sun ~100 000
    glm::vec3 color{1.0f};
    float angularSizeDeg = 0.53f;             // the sun's disc; > 0 gives soft shadows (an M1 technique)
};

} // namespace ghost::world
