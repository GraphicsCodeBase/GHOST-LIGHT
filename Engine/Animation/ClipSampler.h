// Writes one clip's channels into a pose's local transforms at a given time (the interpolation itself is PoseMath's).
#pragma once

#include "Animation/Pose.h"
#include "Assets/AnimationData.h"

namespace ghost::animation {

class ClipSampler {
public:
    // Nodes the clip doesn't animate keep whatever pose.local already holds (normally the rest pose).
    static void apply(const assets::AnimationData& clip, float time, Pose& pose);
    // Wraps (loop) or clamps `time` into the clip's [0, duration] range.
    static float wrapTime(const assets::AnimationData& clip, float time, bool loop);
};

} // namespace ghost::animation
