// AnimationSystem: advances every Animator's clip and poses its skeleton (Animation module) each frame; also hands out
// the skeleton as world-space line segments for the debug overlay.
#pragma once

#include "Animation/Skeleton.h"
#include "Assets/AssetRegistry.h"

#include <unordered_map>
#include <utility>
#include <vector>

#include <entt/entity/fwd.hpp>
#include <glm/vec3.hpp>

namespace ghost::world {

class AnimationSystem {
public:
    void update(entt::registry& registry, const assets::AssetRegistry& assets, float deltaSeconds);
    // Forget cached skeletons (call when a scene is (re)loaded; models may have been reimported).
    void clear() { m_skeletons.clear(); }

    // One segment per joint whose parent is also a joint of the skin, in world space.
    static std::vector<std::pair<glm::vec3, glm::vec3>> skeletonLines(const entt::registry& registry, const AnimationSystem& system);

    // The cached skeleton of a model, or nullptr if it hasn't been built (or can't be).
    const animation::Skeleton* skeleton(assets::ModelHandle model) const;

private:
    const animation::Skeleton* skeletonFor(assets::ModelHandle model, const assets::AssetRegistry& assets);

    std::unordered_map<assets::ModelHandle, animation::Skeleton> m_skeletons;
};

} // namespace ghost::world
