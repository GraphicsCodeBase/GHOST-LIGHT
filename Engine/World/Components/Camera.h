// Camera component: projection settings. The view comes from the entity's transform (looking down -Z, +Y up).
#pragma once

namespace ghost::world {

struct Camera {
    float fovYDeg = 60.0f;
    float nearPlane = 0.05f; // meters; the far plane is at infinity (reverse-Z)
};

} // namespace ghost::world
