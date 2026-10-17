// An animation clip as stored in a glTF file: per animated node property, the key times and the key values.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ghost::assets {

enum class AnimationPath { Translation, Rotation, Scale };
enum class AnimationInterpolation { Step, Linear, CubicSpline };

// One animated property of one node. Values are packed floats: 3 per key for translation/scale, 4 per key for
// rotation (quaternion x, y, z, w, glTF order). CubicSpline keys hold in-tangent, value, out-tangent (3 entries each).
struct AnimationChannelData {
    uint32_t node = 0; // into ModelData::hierarchy
    AnimationPath path = AnimationPath::Translation;
    AnimationInterpolation interpolation = AnimationInterpolation::Linear;
    std::vector<float> times;  // seconds, increasing
    std::vector<float> values;
};

struct AnimationData {
    std::string name;
    float duration = 0.0f; // seconds: the last key time over all channels
    std::vector<AnimationChannelData> channels;
};

} // namespace ghost::assets
