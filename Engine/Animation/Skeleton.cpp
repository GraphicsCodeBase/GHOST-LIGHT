// Skeleton: copies the node tree out of the model and orders it so a single pass can compose parents before children.
#include "Animation/Skeleton.h"

#include <queue>

namespace ghost::animation {

Skeleton Skeleton::build(const assets::ModelData& model, std::string& error) {
    Skeleton skeleton;
    if (model.hierarchy.empty()) {
        error = model.name + ": no node hierarchy (only glTF models can be animated)";
        return skeleton;
    }

    const size_t count = model.hierarchy.size();
    skeleton.m_parents.resize(count);
    skeleton.m_names.resize(count);
    skeleton.m_rest.resize(count);
    skeleton.m_referenceRestGlobals.resize(count);
    std::vector<std::vector<uint32_t>> children(count);
    for (size_t i = 0; i < count; ++i) {
        const assets::HierarchyNodeData& node = model.hierarchy[i];
        skeleton.m_parents[i] = node.parent;
        skeleton.m_names[i] = node.name;
        skeleton.m_rest[i] = {node.translation, node.rotation, node.scale};
        skeleton.m_referenceRestGlobals[i] = node.restGlobal;
        if (node.parent >= 0) {
            children[static_cast<size_t>(node.parent)].push_back(static_cast<uint32_t>(i));
        }
    }

    // Breadth-first from the roots: a node is only queued once its parent has been placed.
    std::queue<uint32_t> pending;
    for (size_t i = 0; i < count; ++i) {
        if (skeleton.m_parents[i] < 0) {
            pending.push(static_cast<uint32_t>(i));
        }
    }
    while (!pending.empty()) {
        const uint32_t node = pending.front();
        pending.pop();
        skeleton.m_order.push_back(node);
        for (uint32_t child : children[node]) {
            pending.push(child);
        }
    }

    if (!model.skins.empty()) {
        skeleton.m_skin = model.skins.front();
        skeleton.m_hasSkin = true;
    }
    return skeleton;
}

Pose Skeleton::makeRestPose() const {
    Pose pose;
    pose.local = m_rest;
    pose.global.assign(m_parents.size(), glm::mat4(1.0f));
    pose.skinning.assign(m_hasSkin ? m_skin.joints.size() : 0, glm::mat4(1.0f));
    return pose;
}

} // namespace ghost::animation
