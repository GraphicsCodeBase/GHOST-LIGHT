// One character's pose this frame: local transforms per node, model-space matrices per node, skinning matrices per joint.
#pragma once

#include "Animation/JointTransform.h"

#include <vector>

#include <glm/mat4x4.hpp>

namespace ghost::animation {

struct Pose {
    std::vector<JointTransform> local; // per hierarchy node, relative to its parent (what clips write)
    std::vector<glm::mat4> global;     // per hierarchy node: node space -> model space
    std::vector<glm::mat4> skinning;   // per joint of the skin: bind space -> model space (what the GPU skins with)
};

} // namespace ghost::animation
