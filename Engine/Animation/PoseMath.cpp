// PoseMath: the user's Stage 1 animation math. Until each CHAR-TODO is replaced, the placeholders below keep the
// character in its rest pose without doing any of the math the user is going to write.
#include "Animation/PoseMath.h"

#include "Animation/Skeleton.h"
#include "Core/Assert.h"

#include <cmath>

namespace ghost::animation {

namespace {

// Placeholder helper: where key 0's value starts (CubicSpline keys store in-tangent, value, out-tangent).
size_t firstValueOffset(const assets::AnimationChannelData& channel, size_t components) {
    return channel.interpolation == assets::AnimationInterpolation::CubicSpline ? components : 0;
}

} // namespace

glm::vec3 PoseMath::sampleVec3(const assets::AnimationChannelData& channel, float /*time*/) {
    // CHAR-TODO(03 Keyframe sampling): find the two keys around `time` and interpolate (Step / Linear; CubicSpline later).
    // Placeholder: always the first key, so the clip never moves.
    const size_t offset = firstValueOffset(channel, 3);
    if (channel.values.size() < offset + 3) {
        return glm::vec3(0.0f);
    }
    return glm::vec3(channel.values[offset], channel.values[offset + 1], channel.values[offset + 2]);
}

glm::quat PoseMath::sampleQuat(const assets::AnimationChannelData& channel, float /*time*/) {
    // CHAR-TODO(03 Keyframe sampling): same as sampleVec3, but rotations need slerp or nlerp (and the shortest path).
    // Placeholder: always the first key.
    const size_t offset = firstValueOffset(channel, 4);
    if (channel.values.size() < offset + 4) {
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }
    const float* v = channel.values.data() + offset;
    return glm::normalize(glm::quat(v[3], v[0], v[1], v[2])); // glTF stores x y z w, glm's constructor takes w first
}

glm::mat4 PoseMath::localMatrix(const JointTransform& transform) {
    // Node -> parent matrix in glTF order T * R * S: scale, then rotate, then translate (study note 01).
      // Expects a unit quaternion (sampleQuat and the rest pose guarantee it); mat4_cast would skew a non-unit one.
    glm::mat4 translate = glm::translate(glm::mat4(1.0f), transform.translation);
    glm::mat4 rotation = glm::mat4_cast(transform.rotation);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), transform.scale);
    GHOST_ASSERT(std::abs(glm::length(transform.rotation) - 1.0f) < 1e-3f, "localMatrix: rotation must be a unit quaternion");
    return translate * rotation * scale;
}

void PoseMath::computeGlobalPose(const Skeleton& skeleton, Pose& pose) {
    // CHAR-TODO(01 Transform hierarchy): global = parent's global * local, in skeleton.evaluationOrder().
    // Placeholder: copies cgltf's rest-pose matrices and ignores pose.local, so animation has no visible effect yet.
    pose.global = skeleton.referenceRestGlobals();
}

void PoseMath::computeSkinningMatrices(const Pose& /*pose*/, const assets::SkinData& skin, std::vector<glm::mat4>& skinning) {
    // CHAR-TODO(02 Inverse bind matrices): combine each joint's global matrix with its inverse bind matrix.
    // Placeholder: identity per joint, which leaves every vertex where the mesh file put it (the bind pose).
    skinning.assign(skin.joints.size(), glm::mat4(1.0f));
}

} // namespace ghost::animation
