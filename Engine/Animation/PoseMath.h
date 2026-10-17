// The core animation math of Stage 1, written by the user (see Design_Vault/Study/Character/01-05). Every function here
// starts as a CHAR-TODO placeholder that keeps the engine running (the character holds its rest pose).
#pragma once

#include "Animation/JointTransform.h"
#include "Animation/Pose.h"
#include "Assets/AnimationData.h"
#include "Assets/ModelData.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace ghost::animation {

class Skeleton;

class PoseMath {
public:
    // Study note 03 (keyframe sampling). The channel's value at `time` seconds, for translation or scale channels.
    // Keys: channel.times (increasing), channel.values (3 floats per key; CubicSpline: in-tangent, value, out-tangent).
    // Before the first key / after the last key: hold the first / last value.
    static glm::vec3 sampleVec3(const assets::AnimationChannelData& channel, float time);
    // Same for rotation channels (4 floats per key, x y z w). Must return a unit quaternion.
    static glm::quat sampleQuat(const assets::AnimationChannelData& channel, float time);

    // Study note 01 (transform hierarchy). The node -> parent matrix of one local transform.
    static glm::mat4 localMatrix(const JointTransform& transform);
    // Study note 01. Fills pose.global (node -> model space) for every node from pose.local, walking
    // skeleton.evaluationOrder() so every parent's matrix is ready before its children need it.
    static void computeGlobalPose(const Skeleton& skeleton, Pose& pose);

    // Study note 02 (inverse bind matrices). Fills pose.skinning: one matrix per joint of `skin` that takes a vertex
    // from bind space to model space in the current pose. skin.joints[j] is the joint's node, skin.inverseBindMatrices[j]
    // its inverse bind matrix.
    static void computeSkinningMatrices(const Pose& pose, const assets::SkinData& skin, std::vector<glm::mat4>& skinning);
};

} // namespace ghost::animation
