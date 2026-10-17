// Skeleton overlay (F4): draws animated skeletons as lines over the image, projected with the renderer's camera.
#pragma once

#include <utility>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace ghost::ui {

class SkeletonOverlay {
public:
    // lines: world-space segments (parent joint -> joint). viewProjection: the camera the frame renders with.
    static void draw(const std::vector<std::pair<glm::vec3, glm::vec3>>& lines, const glm::mat4& viewProjection);
};

} // namespace ghost::ui
