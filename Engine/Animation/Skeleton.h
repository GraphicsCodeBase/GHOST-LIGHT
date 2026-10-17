// A model's node tree prepared for posing: parent indices, a parents-before-children evaluation order, the rest pose,
// and which skin (if any) deforms the mesh. Built once per model and shared by every entity that uses it.
#pragma once

#include "Animation/Pose.h"
#include "Assets/ModelData.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ghost::animation {

class Skeleton {
public:
    // Fails (empty skeleton, error filled) when the model has no node hierarchy.
    static Skeleton build(const assets::ModelData& model, std::string& error);

    size_t nodeCount() const { return m_parents.size(); }
    bool empty() const { return m_parents.empty(); }
    const std::vector<int32_t>& parents() const { return m_parents; }        // -1 = root
    const std::vector<uint32_t>& evaluationOrder() const { return m_order; } // every parent appears before its children
    const std::vector<std::string>& names() const { return m_names; }
    const std::vector<JointTransform>& restPose() const { return m_rest; }
    // Model-space matrices of the rest pose as computed by cgltf: the reference PoseMath::computeGlobalPose is tested against.
    const std::vector<glm::mat4>& referenceRestGlobals() const { return m_referenceRestGlobals; }
    // The skin that deforms the model (the first one; more are reported as a warning), or nullptr. A copy, so the
    // skeleton never points into a ModelData that could be unloaded.
    const assets::SkinData* skin() const { return m_hasSkin ? &m_skin : nullptr; }

    // A pose with the rest transforms and room for every matrix.
    Pose makeRestPose() const;

private:
    std::vector<int32_t> m_parents;
    std::vector<uint32_t> m_order;
    std::vector<std::string> m_names;
    std::vector<JointTransform> m_rest;
    std::vector<glm::mat4> m_referenceRestGlobals;
    assets::SkinData m_skin;
    bool m_hasSkin = false;
};

} // namespace ghost::animation
