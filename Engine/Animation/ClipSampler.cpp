// ClipSampler: routes every channel of a clip to the right part of the right node's local transform.
#include "Animation/ClipSampler.h"

#include "Animation/PoseMath.h"

#include <algorithm>
#include <cmath>

namespace ghost::animation {

void ClipSampler::apply(const assets::AnimationData& clip, float time, Pose& pose) {
    for (const assets::AnimationChannelData& channel : clip.channels) {
        if (channel.node >= pose.local.size() || channel.times.empty()) {
            continue;
        }
        JointTransform& local = pose.local[channel.node];
        switch (channel.path) {
        case assets::AnimationPath::Translation: local.translation = PoseMath::sampleVec3(channel, time); break;
        case assets::AnimationPath::Rotation: local.rotation = PoseMath::sampleQuat(channel, time); break;
        case assets::AnimationPath::Scale: local.scale = PoseMath::sampleVec3(channel, time); break;
        }
    }
}

float ClipSampler::wrapTime(const assets::AnimationData& clip, float time, bool loop) {
    if (clip.duration <= 0.0f) {
        return 0.0f;
    }
    if (loop) {
        const float wrapped = std::fmod(time, clip.duration);
        return wrapped < 0.0f ? wrapped + clip.duration : wrapped;
    }
    return std::clamp(time, 0.0f, clip.duration);
}

} // namespace ghost::animation
