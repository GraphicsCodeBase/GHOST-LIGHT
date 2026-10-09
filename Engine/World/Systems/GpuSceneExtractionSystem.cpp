// GpuSceneExtractionSystem: ECS views -> GpuScene inputs. Lights use physical units straight from the components.
#include "World/Systems/GpuSceneExtractionSystem.h"

#include "World/Components/DirectionalLight.h"
#include "World/Components/MaterialOverride.h"
#include "World/Components/MeshRenderer.h"
#include "World/Components/PointLight.h"
#include "World/Components/SpotLight.h"
#include "World/Components/WorldTransform.h"
#include "World/SceneSettings.h"

#include <cmath>

#include <entt/entity/registry.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

namespace ghost::world {

namespace {

using graphics::scene::GpuLight;
using graphics::scene::GpuScene;

glm::vec3 positionOf(const entt::registry& registry, entt::entity entity) {
    const WorldTransform* world = registry.try_get<WorldTransform>(entity);
    return world ? glm::vec3(world->matrix[3]) : glm::vec3(0.0f);
}

// An entity's -Z axis in world space (where cameras and spot lights point).
glm::vec3 forwardOf(const entt::registry& registry, entt::entity entity) {
    const WorldTransform* world = registry.try_get<WorldTransform>(entity);
    return world ? -glm::normalize(glm::vec3(world->matrix[2])) : glm::vec3(0.0f, 0.0f, -1.0f);
}

} // namespace

GpuScene::ModelId GpuSceneExtractionSystem::gpuModel(assets::ModelHandle handle, assets::AssetRegistry& assets, GpuScene& scene) {
    if (handle == assets::kInvalidModel) {
        return GpuScene::kInvalidModel;
    }
    if (handle >= m_models.size()) {
        m_models.resize(static_cast<size_t>(handle) + 1, GpuScene::kInvalidModel);
    }
    if (m_models[handle] == GpuScene::kInvalidModel) {
        assets::ModelData* model = assets.model(handle);
        if (!model) {
            return GpuScene::kInvalidModel;
        }
        m_models[handle] = scene.uploadModel(*model);
        // The GPU has the texels now; keep only the image descriptions on the CPU.
        for (assets::ImageData& image : model->images) {
            image.pixels.clear();
            image.pixels.shrink_to_fit();
        }
    }
    return m_models[handle];
}

void GpuSceneExtractionSystem::update(entt::registry& registry, const SceneSettings& settings, assets::AssetRegistry& assets,
                                      GpuScene& scene) {
    // The registry was cleared: every handle we mapped is stale.
    if (assets.modelCount() < m_models.size()) {
        scene.clearModels();
        m_models.clear();
        m_environmentUploaded = false;
    }

    if (!m_environmentUploaded || m_environment != settings.environment) {
        scene.setEnvironment(assets.environment(settings.environment), settings.environmentIntensity);
        m_environment = settings.environment;
        m_environmentUploaded = true;
    }
    scene.setEnvironmentIntensity(settings.environmentIntensity);

    std::vector<GpuScene::InstanceInput> instances;
    for (const auto [entity, renderer, world] : registry.view<const MeshRenderer, const WorldTransform>().each()) {
        if (!renderer.visible) {
            continue;
        }
        GpuScene::InstanceInput instance;
        instance.model = gpuModel(renderer.handle, assets, scene);
        if (instance.model == GpuScene::kInvalidModel) {
            continue;
        }
        instance.world = world.matrix;
        instance.previousWorld = world.previous;
        instance.entityId = static_cast<uint32_t>(entt::to_integral(entity)) + 1;
        if (const MaterialOverride* materialOverride = registry.try_get<MaterialOverride>(entity)) {
            using namespace graphics::scene;
            if (materialOverride->baseColor) {
                instance.overrideFlags |= kOverrideBaseColor;
                instance.baseColor = *materialOverride->baseColor;
            }
            if (materialOverride->roughness) {
                instance.overrideFlags |= kOverrideRoughness;
                instance.roughness = *materialOverride->roughness;
            }
            if (materialOverride->metallic) {
                instance.overrideFlags |= kOverrideMetallic;
                instance.metallic = *materialOverride->metallic;
            }
            if (materialOverride->emissive) {
                instance.overrideFlags |= kOverrideEmissive;
                instance.emissive = *materialOverride->emissive;
            }
        }
        instances.push_back(instance);
    }
    scene.setInstances(instances);

    // The first directional light is the sun (one sun per scene; more would need a light list of their own).
    GpuScene::SunInput sun;
    const entt::entity sunEntity = registry.view<const DirectionalLight>().front();
    if (sunEntity != entt::null) {
        const DirectionalLight& light = registry.get<const DirectionalLight>(sunEntity);
        sun.enabled = true;
        sun.directionToSun = -glm::normalize(light.direction);
        sun.illuminance = light.illuminance;
        sun.color = light.color;
        sun.angularRadius = glm::radians(light.angularSizeDeg) * 0.5f;
    }

    std::vector<GpuLight> lights;
    for (const auto [entity, light] : registry.view<const PointLight>().each()) {
        GpuLight gpu{};
        gpu.type = graphics::scene::kLightPoint;
        gpu.position = positionOf(registry, entity);
        gpu.direction = glm::vec3(0.0f, -1.0f, 0.0f);
        gpu.intensity = light.color * light.intensity;
        gpu.radius = light.radius;
        gpu.range = light.range;
        gpu.cosInner = -1.0f;
        gpu.cosOuter = -1.0f;
        lights.push_back(gpu);
    }
    for (const auto [entity, light] : registry.view<const SpotLight>().each()) {
        GpuLight gpu{};
        gpu.type = graphics::scene::kLightSpot;
        gpu.position = positionOf(registry, entity);
        gpu.direction = forwardOf(registry, entity);
        gpu.intensity = light.color * light.intensity;
        gpu.radius = light.radius;
        gpu.range = light.range;
        // Cone angles are measured from the axis (glTF convention).
        gpu.cosInner = std::cos(glm::radians(light.innerConeDeg));
        gpu.cosOuter = std::cos(glm::radians(light.outerConeDeg));
        lights.push_back(gpu);
    }
    scene.setLights(sun, std::move(lights));
}

} // namespace ghost::world
