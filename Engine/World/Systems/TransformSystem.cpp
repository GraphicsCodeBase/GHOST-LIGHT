// World matrices = parent world * translate * rotate * scale. Parents are resolved recursively (hierarchies are shallow).
#include "World/Systems/TransformSystem.h"

#include "World/Components/Transform.h"
#include "World/Components/WorldTransform.h"

#include <unordered_map>

#include <entt/entity/registry.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ghost::world {

namespace {

glm::mat4 localMatrix(const Transform& transform) {
    return glm::translate(glm::mat4(1.0f), transform.position) * glm::mat4_cast(transform.rotation) *
           glm::scale(glm::mat4(1.0f), transform.scale);
}

glm::mat4 worldMatrix(entt::registry& registry, entt::entity entity, std::unordered_map<entt::entity, glm::mat4>& computed, int depth) {
    const auto found = computed.find(entity);
    if (found != computed.end()) {
        return found->second;
    }
    const Transform& transform = registry.get<Transform>(entity);
    glm::mat4 matrix = localMatrix(transform);
    // Depth guard: a parent cycle (impossible from scene files, possible from bad edits) stops instead of recursing forever.
    if (transform.parent != entt::null && depth < 32 && registry.valid(transform.parent) && registry.all_of<Transform>(transform.parent)) {
        matrix = worldMatrix(registry, transform.parent, computed, depth + 1) * matrix;
    }
    computed.emplace(entity, matrix);
    return matrix;
}

} // namespace

void TransformSystem::update(entt::registry& registry) {
    std::unordered_map<entt::entity, glm::mat4> computed;
    for (const entt::entity entity : registry.view<Transform>()) {
        const glm::mat4 matrix = worldMatrix(registry, entity, computed, 0);
        if (WorldTransform* world = registry.try_get<WorldTransform>(entity)) {
            world->previous = world->matrix;
            world->matrix = matrix;
        } else {
            registry.emplace<WorldTransform>(entity, WorldTransform{matrix, matrix});
        }
    }
}

} // namespace ghost::world
