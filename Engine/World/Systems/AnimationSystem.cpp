// AnimationSystem: rest pose -> clip -> global pose -> skinning matrices, per animated entity, every frame.
#include "World/Systems/AnimationSystem.h"

#include "Animation/ClipSampler.h"
#include "Animation/PoseMath.h"
#include "Core/Log.h"
#include "World/Components/Animator.h"
#include "World/Components/MeshRenderer.h"
#include "World/Components/WorldTransform.h"

#include <algorithm>

#include <entt/entity/registry.hpp>

namespace ghost::world {

const animation::Skeleton* AnimationSystem::skeletonFor(assets::ModelHandle model, const assets::AssetRegistry& assets) {
    const auto found = m_skeletons.find(model);
    if (found != m_skeletons.end()) {
        return found->second.empty() ? nullptr : &found->second;
    }
    const assets::ModelData* data = assets.model(model);
    if (!data) {
        return nullptr;
    }
    std::string error;
    animation::Skeleton skeleton = animation::Skeleton::build(*data, error);
    if (skeleton.empty()) {
        core::Log::warning("Animator on {}: {}", data->reference, error);
    } else if (data->skins.size() > 1) {
        core::Log::warning("{} has {} skins; only the first one is animated", data->reference, data->skins.size());
    }
    // Failures are cached as an empty skeleton so the warning appears once, not every frame.
    const auto inserted = m_skeletons.emplace(model, std::move(skeleton)).first;
    return inserted->second.empty() ? nullptr : &inserted->second;
}

const animation::Skeleton* AnimationSystem::skeleton(assets::ModelHandle model) const {
    const auto found = m_skeletons.find(model);
    return found != m_skeletons.end() && !found->second.empty() ? &found->second : nullptr;
}

void AnimationSystem::update(entt::registry& registry, const assets::AssetRegistry& assets, float deltaSeconds) {
    for (const auto [entity, animator, renderer] : registry.view<Animator, const MeshRenderer>().each()) {
        const animation::Skeleton* skeleton = skeletonFor(renderer.handle, assets);
        const assets::ModelData* model = assets.model(renderer.handle);
        if (!skeleton || !model) {
            continue;
        }

        // Resolve the clip by name once (scene reloads create a fresh Animator, so this re-runs after edits).
        if (animator.clipIndex < 0 && !model->animations.empty()) {
            if (animator.clip.empty()) {
                animator.clipIndex = 0;
            } else {
                const auto match = std::find_if(model->animations.begin(), model->animations.end(),
                                                [&](const assets::AnimationData& clip) { return clip.name == animator.clip; });
                if (match != model->animations.end()) {
                    animator.clipIndex = static_cast<int32_t>(match - model->animations.begin());
                } else {
                    core::Log::warning("{}: no animation named '{}', playing '{}' instead", model->reference, animator.clip,
                                       model->animations.front().name);
                    animator.clipIndex = 0;
                }
            }
        }

        animator.pose = skeleton->makeRestPose();
        if (animator.clipIndex >= 0) {
            const assets::AnimationData& clip = model->animations[static_cast<size_t>(animator.clipIndex)];
            if (animator.playing) {
                animator.time = animation::ClipSampler::wrapTime(clip, animator.time + deltaSeconds * animator.speed, animator.loop);
            }
            animation::ClipSampler::apply(clip, animator.time, animator.pose);
        }
        animation::PoseMath::computeGlobalPose(*skeleton, animator.pose);
        if (const assets::SkinData* skin = skeleton->skin()) {
            animation::PoseMath::computeSkinningMatrices(animator.pose, *skin, animator.pose.skinning);
        }
    }
}

std::vector<std::pair<glm::vec3, glm::vec3>> AnimationSystem::skeletonLines(const entt::registry& registry, const AnimationSystem& system) {
    std::vector<std::pair<glm::vec3, glm::vec3>> lines;
    for (const auto [entity, animator, renderer, world] : registry.view<const Animator, const MeshRenderer, const WorldTransform>().each()) {
        const animation::Skeleton* skeleton = system.skeleton(renderer.handle);
        const assets::SkinData* skin = skeleton ? skeleton->skin() : nullptr;
        if (!skin || animator.pose.global.size() != skeleton->nodeCount()) {
            continue;
        }
        std::vector<bool> isJoint(skeleton->nodeCount(), false);
        for (uint32_t joint : skin->joints) {
            isJoint[joint] = true;
        }
        for (uint32_t joint : skin->joints) {
            const int32_t parent = skeleton->parents()[joint];
            if (parent < 0 || !isJoint[static_cast<size_t>(parent)]) {
                continue;
            }
            const glm::vec3 from = glm::vec3(world.matrix * animator.pose.global[static_cast<size_t>(parent)][3]);
            const glm::vec3 to = glm::vec3(world.matrix * animator.pose.global[joint][3]);
            lines.emplace_back(from, to);
        }
    }
    return lines;
}

} // namespace ghost::world
