// Procedural geometry: quads, boxes and a UV sphere assembled into ModelData (one primitive per material).
#include "Assets/ProceduralMeshes.h"

#include <cmath>
#include <limits>
#include <map>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ghost::assets {

namespace {

// Collects triangles per material, then lays them out as one primitive per material in a single mesh.
class Builder {
public:
    uint32_t addMaterial(MaterialData material) {
        m_model.materials.push_back(std::move(material));
        return static_cast<uint32_t>(m_model.materials.size() - 1);
    }

    // A quad facing `normal` (counter-clockwise seen from that side): center c, half-extents along u and v, cross(u, v) = normal.
    void addQuad(uint32_t material, glm::vec3 c, glm::vec3 u, glm::vec3 v, glm::vec2 uvScale = glm::vec2(1.0f)) {
        Part& part = m_parts[material];
        const glm::vec3 normal = glm::normalize(glm::cross(u, v));
        const glm::vec4 tangent(glm::normalize(u), 1.0f);
        const uint32_t base = static_cast<uint32_t>(part.vertices.size());
        const glm::vec3 corners[4] = {c - u - v, c + u - v, c + u + v, c - u + v};
        const glm::vec2 uvs[4] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
        for (int i = 0; i < 4; ++i) {
            part.vertices.push_back({corners[i], normal, tangent, uvs[i] * uvScale});
        }
        for (uint32_t index : {0u, 1u, 2u, 0u, 2u, 3u}) {
            part.indices.push_back(base + index);
        }
    }

    // A box with outward-facing sides, rotated around Y by `yawDegrees`.
    void addBox(uint32_t material, glm::vec3 center, glm::vec3 halfSize, float yawDegrees) {
        const glm::mat3 rotation = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(yawDegrees), glm::vec3(0, 1, 0)));
        const glm::vec3 x = rotation * glm::vec3(halfSize.x, 0, 0);
        const glm::vec3 y = rotation * glm::vec3(0, halfSize.y, 0);
        const glm::vec3 z = rotation * glm::vec3(0, 0, halfSize.z);
        addQuad(material, center + y, x, -z);  // top (+Y)
        addQuad(material, center - y, x, z);   // bottom (-Y)
        addQuad(material, center + x, -z, y);  // +X
        addQuad(material, center - x, z, y);   // -X
        addQuad(material, center + z, x, y);   // +Z
        addQuad(material, center - z, -x, y);  // -Z
    }

    void addSphere(uint32_t material, float radius, uint32_t segments, uint32_t rings) {
        Part& part = m_parts[material];
        const uint32_t base = static_cast<uint32_t>(part.vertices.size());
        for (uint32_t r = 0; r <= rings; ++r) {
            const float theta = glm::pi<float>() * static_cast<float>(r) / static_cast<float>(rings);
            for (uint32_t s = 0; s <= segments; ++s) {
                const float phi = glm::two_pi<float>() * static_cast<float>(s) / static_cast<float>(segments);
                const glm::vec3 n(std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi));
                const glm::vec4 tangent(-std::sin(phi), 0.0f, std::cos(phi), 1.0f);
                const glm::vec2 uv(static_cast<float>(s) / static_cast<float>(segments), static_cast<float>(r) / static_cast<float>(rings));
                part.vertices.push_back({n * radius, n, tangent, uv});
            }
        }
        for (uint32_t r = 0; r < rings; ++r) {
            for (uint32_t s = 0; s < segments; ++s) {
                const uint32_t a = base + r * (segments + 1) + s;
                const uint32_t b = a + segments + 1;
                for (uint32_t index : {a, a + 1, b, a + 1, b + 1, b}) {
                    part.indices.push_back(index);
                }
            }
        }
    }

    ModelData finish(std::string name) {
        MeshData mesh;
        mesh.name = name;
        m_model.boundsMin = glm::vec3(std::numeric_limits<float>::max());
        m_model.boundsMax = glm::vec3(-std::numeric_limits<float>::max());
        for (auto& [material, part] : m_parts) {
            PrimitiveData primitive;
            primitive.material = material;
            primitive.firstVertex = static_cast<uint32_t>(m_model.vertices.size());
            primitive.vertexCount = static_cast<uint32_t>(part.vertices.size());
            primitive.firstIndex = static_cast<uint32_t>(m_model.indices.size());
            primitive.indexCount = static_cast<uint32_t>(part.indices.size());
            primitive.boundsMin = glm::vec3(std::numeric_limits<float>::max());
            primitive.boundsMax = glm::vec3(-std::numeric_limits<float>::max());
            for (const Vertex& vertex : part.vertices) {
                primitive.boundsMin = glm::min(primitive.boundsMin, vertex.position);
                primitive.boundsMax = glm::max(primitive.boundsMax, vertex.position);
            }
            m_model.boundsMin = glm::min(m_model.boundsMin, primitive.boundsMin);
            m_model.boundsMax = glm::max(m_model.boundsMax, primitive.boundsMax);
            m_model.vertices.insert(m_model.vertices.end(), part.vertices.begin(), part.vertices.end());
            m_model.indices.insert(m_model.indices.end(), part.indices.begin(), part.indices.end());
            mesh.primitives.push_back(static_cast<uint32_t>(m_model.primitives.size()));
            m_model.primitives.push_back(primitive);
        }
        m_model.meshes.push_back(std::move(mesh));
        m_model.nodes.push_back({name, 0, glm::mat4(1.0f)});
        m_model.name = name;
        m_model.reference = std::string(ProceduralMeshes::kPrefix) + name;
        return std::move(m_model);
    }

private:
    struct Part {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };
    ModelData m_model;
    std::map<uint32_t, Part> m_parts;
};

MaterialData diffuse(const char* name, glm::vec3 color) {
    MaterialData material;
    material.name = name;
    material.baseColorFactor = glm::vec4(color, 1.0f);
    material.metallicFactor = 0.0f;
    material.roughnessFactor = 1.0f;
    return material;
}

// The classic Cornell box, 2 m wide: floor at y = 0, ceiling at y = 2, open towards +Z. Colors and light emission
// follow the original measured scene (white 0.73, red 0.65/0.05/0.05, green 0.12/0.45/0.15, light 17/12/4).
ModelData cornellBox() {
    Builder builder;
    const uint32_t white = builder.addMaterial(diffuse("White", {0.73f, 0.73f, 0.73f}));
    const uint32_t red = builder.addMaterial(diffuse("Red", {0.65f, 0.05f, 0.05f}));
    const uint32_t green = builder.addMaterial(diffuse("Green", {0.12f, 0.45f, 0.15f}));
    MaterialData lightMaterial = diffuse("Light", {0.78f, 0.78f, 0.78f});
    lightMaterial.emissiveFactor = {17.0f, 12.0f, 4.0f};
    const uint32_t light = builder.addMaterial(lightMaterial);

    builder.addQuad(white, {0, 0, 0}, {1, 0, 0}, {0, 0, -1});   // floor, faces +Y
    builder.addQuad(white, {0, 2, 0}, {1, 0, 0}, {0, 0, 1});    // ceiling, faces -Y
    builder.addQuad(white, {0, 1, -1}, {1, 0, 0}, {0, 1, 0});   // back wall, faces +Z
    builder.addQuad(red, {-1, 1, 0}, {0, 0, -1}, {0, 1, 0});    // left wall, faces +X
    builder.addQuad(green, {1, 1, 0}, {0, 0, 1}, {0, 1, 0});    // right wall, faces -X
    builder.addQuad(light, {0, 1.998f, 0}, {0.25f, 0, 0}, {0, 0, 0.22f}); // ceiling light, faces -Y
    builder.addBox(white, {0.33f, 0.3f, 0.28f}, {0.3f, 0.3f, 0.3f}, -17.0f);  // short block
    builder.addBox(white, {-0.33f, 0.6f, -0.3f}, {0.3f, 0.6f, 0.3f}, 17.0f);  // tall block
    return builder.finish("CornellBox");
}

ModelData cube() {
    Builder builder;
    const uint32_t material = builder.addMaterial(diffuse("Default", {0.8f, 0.8f, 0.8f}));
    builder.addBox(material, {0, 0, 0}, {0.5f, 0.5f, 0.5f}, 0.0f);
    return builder.finish("Cube");
}

ModelData sphere() {
    Builder builder;
    const uint32_t material = builder.addMaterial(diffuse("Default", {0.8f, 0.8f, 0.8f}));
    builder.addSphere(material, 0.5f, 64, 32);
    return builder.finish("Sphere");
}

ModelData plane() {
    Builder builder;
    const uint32_t material = builder.addMaterial(diffuse("Default", {0.8f, 0.8f, 0.8f}));
    builder.addQuad(material, {0, 0, 0}, {5, 0, 0}, {0, 0, -5}, glm::vec2(10.0f));
    return builder.finish("Plane");
}

} // namespace

std::optional<ModelData> ProceduralMeshes::create(std::string_view reference) {
    if (!isProcedural(reference)) {
        return std::nullopt;
    }
    const std::string_view name = reference.substr(kPrefix.size());
    if (name == "CornellBox") return cornellBox();
    if (name == "Cube") return cube();
    if (name == "Sphere") return sphere();
    if (name == "Plane") return plane();
    return std::nullopt;
}

std::vector<std::string_view> ProceduralMeshes::names() {
    return {"procedural:CornellBox", "procedural:Cube", "procedural:Sphere", "procedural:Plane"};
}

} // namespace ghost::assets
