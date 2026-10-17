// PoseMath: the user's Stage 1 animation math. Until each CHAR-TODO is replaced, the placeholders below keep the
// character in its rest pose without doing any of the math the user is going to write.
#include "Animation/PoseMath.h"

#include "Animation/Skeleton.h"

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

glm::mat4 PoseMath::localMatrix(const JointTransform& /*transform*/) {
    // CHAR-TODO(01 Transform hierarchy): build the node -> parent matrix from translation, rotation and scale.
    // Placeholder: identity (unused while computeGlobalPose is a placeholder too).
    return glm::mat4(1.0f);
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
