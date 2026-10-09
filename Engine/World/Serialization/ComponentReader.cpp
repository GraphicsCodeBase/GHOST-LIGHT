// Component readers: each validates its fields (unknown keys warn, wrong types error) and fills the struct.
#include "World/Serialization/ComponentReader.h"

#include "Core/JsonReader.h"
#include "World/Components/Camera.h"
#include "World/Components/DirectionalLight.h"
#include "World/Components/MaterialOverride.h"
#include "World/Components/MeshRenderer.h"
#include "World/Components/PointLight.h"
#include "World/Components/SpotLight.h"
#include "World/Components/Transform.h"

#include <algorithm>
#include <array>
#include <string_view>

#include <entt/entity/registry.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ghost::world {

namespace {

constexpr std::array<std::string_view, 7> kKnownComponents = {"MeshRenderer", "MaterialOverride", "DirectionalLight", "PointLight",
                                                              "SpotLight", "Camera", "Transform"};
constexpr std::array<std::string_view, 4> kSandboxComponents = {"RigidBody", "Collider", "Grabbable", "Frozen"};

void readMeshRenderer(const core::JsonReader& data, entt::registry& registry, entt::entity entity, assets::AssetRegistry& assets,
                      std::vector<std::string>& assetWarnings) {
    data.warnUnknownKeys({"model", "visible"});
    MeshRenderer renderer;
    renderer.model = data.string("model");
    renderer.visible = data.boolean("visible", true);
    if (renderer.model.empty()) {
        data.error("needs a \"model\": a path relative to Content/ or \"procedural:CornellBox\"");
    } else {
        std::string error;
        renderer.handle = assets.loadModel(renderer.model, error, &assetWarnings);
        if (renderer.handle == assets::kInvalidModel) {
            data.child("model").error(error);
        }
    }
    registry.emplace_or_replace<MeshRenderer>(entity, renderer);
}

void readMaterialOverride(const core::JsonReader& data, entt::registry& registry, entt::entity entity) {
    data.warnUnknownKeys({"baseColor", "roughness", "metallic", "emissive", "transmission"});
    MaterialOverride material;
    if (data.has("baseColor")) material.baseColor = data.vec3("baseColor", glm::vec3(1.0f));
    if (data.has("roughness")) material.roughness = data.number("roughness", 0.5f);
    if (data.has("metallic")) material.metallic = data.number("metallic", 0.0f);
    if (data.has("emissive")) material.emissive = data.vec3("emissive", glm::vec3(0.0f));
    if (data.has("transmission")) material.transmission = data.number("transmission", 0.0f);
    registry.emplace_or_replace<MaterialOverride>(entity, material);
}

void readDirectionalLight(const core::JsonReader& data, entt::registry& registry, entt::entity entity) {
    data.warnUnknownKeys({"direction", "illuminance", "color", "angularSizeDeg"});
    DirectionalLight light;
    light.direction = data.vec3("direction", light.direction);
    if (glm::dot(light.direction, light.direction) < 1e-12f) {
        data.child("direction").error("must not be zero");
        light.direction = DirectionalLight{}.direction;
    }
    light.direction = glm::normalize(light.direction);
    light.illuminance = data.number("illuminance", light.illuminance);
    light.color = data.vec3("color", light.color);
    light.angularSizeDeg = data.number("angularSizeDeg", light.angularSizeDeg);
    registry.emplace_or_replace<DirectionalLight>(entity, light);
}

void readPointLight(const core::JsonReader& data, entt::registry& registry, entt::entity entity) {
    data.warnUnknownKeys({"color", "intensity", "radius", "range"});
    PointLight light;
    light.color = data.vec3("color", light.color);
    light.intensity = data.number("intensity", light.intensity);
    light.radius = data.number("radius", light.radius);
    light.range = data.number("range", light.range);
    registry.emplace_or_replace<PointLight>(entity, light);
}

void readSpotLight(const core::JsonReader& data, entt::registry& registry, entt::entity entity) {
    data.warnUnknownKeys({"color", "intensity", "radius", "innerConeDeg", "outerConeDeg", "range"});
    SpotLight light;
    light.color = data.vec3("color", light.color);
    light.intensity = data.number("intensity", light.intensity);
    light.radius = data.number("radius", light.radius);
    light.innerConeDeg = data.number("innerConeDeg", light.innerConeDeg);
    light.outerConeDeg = data.number("outerConeDeg", light.outerConeDeg);
    light.range = data.number("range", light.range);
    if (light.innerConeDeg > light.outerConeDeg) {
        data.child("innerConeDeg").error("must not be larger than outerConeDeg");
    }
    registry.emplace_or_replace<SpotLight>(entity, light);
}

void readCamera(const core::JsonReader& data, entt::registry& registry, entt::entity entity) {
    data.warnUnknownKeys({"fovYDeg", "nearPlane"});
    Camera camera;
    camera.fovYDeg = data.number("fovYDeg", camera.fovYDeg);
    camera.nearPlane = data.number("nearPlane", camera.nearPlane);
    registry.emplace_or_replace<Camera>(entity, camera);
}

} // namespace

void ComponentReader::readTransform(const core::JsonReader& data, Transform& transform) {
    if (!data.expectObject("a transform")) {
        return;
    }
    data.warnUnknownKeys({"position", "rotationEuler", "rotation", "scale"});
    transform.position = data.vec3("position", transform.position);
    if (data.has("rotationEuler")) {
        // Degrees: [pitch (X), yaw (Y), roll (Z)].
        transform.rotation = glm::quat(glm::radians(data.vec3("rotationEuler", glm::vec3(0.0f))));
    }
    if (data.has("rotation")) {
        const glm::vec4 q = data.vec4("rotation", glm::vec4(0, 0, 0, 1));
        transform.rotation = glm::normalize(glm::quat(q.w, q.x, q.y, q.z));
    }
    if (data.has("scale")) {
        const core::JsonReader scale = data.child("scale");
        if (scale.raw().is_number()) {
            transform.scale = glm::vec3(scale.raw().get<float>());
        } else {
            transform.scale = data.vec3("scale", transform.scale);
        }
    }
}

void ComponentReader::apply(const std::string& name, const core::JsonReader& data, entt::registry& registry, entt::entity entity,
                            assets::AssetRegistry& assets, std::vector<std::string>& assetWarnings) {
    if (name == "Transform") {
        readTransform(data, registry.get_or_emplace<Transform>(entity));
        return;
    }
    if (!data.expectObject("component \"" + name + "\" settings")) {
        return;
    }
    if (name == "MeshRenderer") {
        readMeshRenderer(data, registry, entity, assets, assetWarnings);
    } else if (name == "MaterialOverride") {
        readMaterialOverride(data, registry, entity);
    } else if (name == "DirectionalLight") {
        readDirectionalLight(data, registry, entity);
    } else if (name == "PointLight") {
        readPointLight(data, registry, entity);
    } else if (name == "SpotLight") {
        readSpotLight(data, registry, entity);
    } else if (name == "Camera") {
        readCamera(data, registry, entity);
    } else if (std::find(kSandboxComponents.begin(), kSandboxComponents.end(), name) != kSandboxComponents.end()) {
        data.warning("arrives with the sandbox (milestone M0b); ignored for now");
    } else {
        std::string known;
        for (std::string_view component : kKnownComponents) {
            known += (known.empty() ? "" : ", ") + std::string(component);
        }
        data.warning("unknown component \"" + name + "\", ignored (known: " + known + ")");
    }
}

} // namespace ghost::world
