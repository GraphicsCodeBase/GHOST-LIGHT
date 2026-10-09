// MaterialOverride component: replaces material values for every primitive of the entity's model (unset = keep the model's).
#pragma once

#include <optional>

#include <glm/vec3.hpp>

namespace ghost::world {

struct MaterialOverride {
    std::optional<glm::vec3> baseColor;
    std::optional<float> roughness;
    std::optional<float> metallic;
    std::optional<glm::vec3> emissive;  // linear radiance multiplier
    std::optional<float> transmission;
};

} // namespace ghost::world
