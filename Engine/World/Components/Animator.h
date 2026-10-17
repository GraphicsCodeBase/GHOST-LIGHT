// Animator component: plays one clip of the entity's MeshRenderer model and holds the resulting pose.
#pragma once

#include "Animation/Pose.h"

#include <cstdint>
#include <string>

namespace ghost::world {

struct Animator {
    std::string clip;      // clip name in the model; empty = the model's first clip
    float speed = 1.0f;    // playback rate (1 = authored speed)
    bool loop = true;
    bool playing = true;
    float time = 0.0f;     // seconds into the clip
    int32_t clipIndex = -1; // resolved by AnimationSystem; -1 = not resolved yet / no such clip
    animation::Pose pose;  // written by AnimationSystem every frame
};

} // namespace ghost::world
