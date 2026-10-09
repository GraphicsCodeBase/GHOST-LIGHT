// GpuSceneExtractionSystem: copies what the GPU needs out of the ECS each frame (instances, lights, environment) into
// the GpuScene as plain data, uploading models the first time an entity uses them.
#pragma once

#include "Assets/AssetRegistry.h"
#include "Graphics/GpuScene/GpuScene.h"

#include <vector>

#include <entt/entity/fwd.hpp>

namespace ghost::world {

struct SceneSettings;

class GpuSceneExtractionSystem {
public:
    // Model and environment uploads block (load time); everything else is a per-frame copy.
    void update(entt::registry& registry, const SceneSettings& settings, assets::AssetRegistry& assets, graphics::scene::GpuScene& scene);

private:
    graphics::scene::GpuScene::ModelId gpuModel(assets::ModelHandle handle, assets::AssetRegistry& assets, graphics::scene::GpuScene& scene);

    std::vector<graphics::scene::GpuScene::ModelId> m_models; // indexed by assets::ModelHandle
    assets::EnvironmentHandle m_environment = assets::kInvalidEnvironment;
    bool m_environmentUploaded = false;
};

} // namespace ghost::world
