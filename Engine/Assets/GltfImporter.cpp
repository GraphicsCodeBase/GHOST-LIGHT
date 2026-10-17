// glTF import: meshes -> one shared vertex/index array, materials (metallic-roughness + common extensions), nodes, images,
// plus the node hierarchy, skins (joints + inverse bind matrices + per-vertex weights) and animation clips.
#include "Assets/GltfImporter.h"

#include "Assets/ImageLoader.h"
#include "Core/Log.h"
#include "Core/Paths.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <execution>
#include <fstream>
#include <iterator>
#include <limits>
#include <numeric>

#include <cgltf.h>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

namespace ghost::assets {

namespace {

// cgltf opens files with fopen (ANSI paths). Route reads through std::ifstream with wide paths instead.
cgltf_result readFile(const cgltf_memory_options* /*memory*/, const cgltf_file_options* /*file*/, const char* path, cgltf_size* size, void** data) {
    std::ifstream stream(core::Paths::fromUtf8(path), std::ios::binary | std::ios::ate);
    if (!stream) {
        return cgltf_result_file_not_found;
    }
    const std::streamsize length = stream.tellg();
    stream.seekg(0);
    void* buffer = std::malloc(static_cast<size_t>(length));
    if (!buffer) {
        return cgltf_result_out_of_memory;
    }
    if (!stream.read(static_cast<char*>(buffer), length)) {
        std::free(buffer);
        return cgltf_result_io_error;
    }
    *size = static_cast<cgltf_size>(length);
    *data = buffer;
    return cgltf_result_success;
}

void releaseFile(const cgltf_memory_options* /*memory*/, const cgltf_file_options* /*file*/, void* data) {
    std::free(data);
}

const char* resultName(cgltf_result result) {
    switch (result) {
    case cgltf_result_data_too_short: return "file is truncated";
    case cgltf_result_unknown_format: return "not a glTF file";
    case cgltf_result_invalid_json: return "invalid JSON";
    case cgltf_result_invalid_gltf: return "invalid glTF";
    case cgltf_result_file_not_found: return "file not found";
    case cgltf_result_io_error: return "read error";
    case cgltf_result_out_of_memory: return "out of memory";
    case cgltf_result_legacy_gltf: return "glTF 1.0 is not supported";
    default: return "error";
    }
}

std::vector<float> unpack(const cgltf_accessor* accessor, size_t components) {
    std::vector<float> values(accessor->count * components);
    cgltf_accessor_unpack_floats(accessor, values.data(), values.size());
    return values;
}

void computeNormals(std::vector<Vertex>& vertices, uint32_t firstVertex, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount) {
    std::vector<glm::vec3> sums(vertexCount, glm::vec3(0.0f));
    for (uint32_t i = 0; i + 2 < indexCount; i += 3) {
        const glm::vec3& p0 = vertices[firstVertex + indices[i]].position;
        const glm::vec3& p1 = vertices[firstVertex + indices[i + 1]].position;
        const glm::vec3& p2 = vertices[firstVertex + indices[i + 2]].position;
        const glm::vec3 faceNormal = glm::cross(p1 - p0, p2 - p0); // length = 2x area: larger faces weigh more
        for (uint32_t k = 0; k < 3; ++k) {
            sums[indices[i + k]] += faceNormal;
        }
    }
    for (uint32_t v = 0; v < vertexCount; ++v) {
        const float length = glm::length(sums[v]);
        vertices[firstVertex + v].normal = length > 0.0f ? sums[v] / length : glm::vec3(0.0f, 1.0f, 0.0f);
    }
}

// Per-triangle tangents from UV derivatives, accumulated per vertex, then orthogonalized against the normal.
void computeTangents(std::vector<Vertex>& vertices, uint32_t firstVertex, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount) {
    std::vector<glm::vec3> tangents(vertexCount, glm::vec3(0.0f));
    std::vector<glm::vec3> bitangents(vertexCount, glm::vec3(0.0f));
    for (uint32_t i = 0; i + 2 < indexCount; i += 3) {
        const Vertex& v0 = vertices[firstVertex + indices[i]];
        const Vertex& v1 = vertices[firstVertex + indices[i + 1]];
        const Vertex& v2 = vertices[firstVertex + indices[i + 2]];
        const glm::vec3 e1 = v1.position - v0.position;
        const glm::vec3 e2 = v2.position - v0.position;
        const glm::vec2 d1 = v1.uv - v0.uv;
        const glm::vec2 d2 = v2.uv - v0.uv;
        const float det = d1.x * d2.y - d2.x * d1.y;
        if (std::abs(det) < 1e-12f) {
            continue;
        }
        const float inv = 1.0f / det;
        const glm::vec3 t = (e1 * d2.y - e2 * d1.y) * inv;
        const glm::vec3 b = (e2 * d1.x - e1 * d2.x) * inv;
        for (uint32_t k = 0; k < 3; ++k) {
            tangents[indices[i + k]] += t;
            bitangents[indices[i + k]] += b;
        }
    }
    for (uint32_t v = 0; v < vertexCount; ++v) {
        Vertex& vertex = vertices[firstVertex + v];
        const glm::vec3 n = vertex.normal;
        glm::vec3 t = tangents[v] - n * glm::dot(n, tangents[v]);
        if (glm::dot(t, t) < 1e-12f) {
            // No usable UVs: any vector perpendicular to the normal.
            t = std::abs(n.x) < 0.9f ? glm::cross(n, glm::vec3(1, 0, 0)) : glm::cross(n, glm::vec3(0, 1, 0));
        }
        t = glm::normalize(t);
        const float handedness = glm::dot(glm::cross(n, t), bitangents[v]) < 0.0f ? -1.0f : 1.0f;
        vertex.tangent = glm::vec4(t, handedness);
    }
}

int32_t imageIndex(const cgltf_texture_view& view, const cgltf_data* data) {
    if (!view.texture || !view.texture->image) {
        return -1;
    }
    return static_cast<int32_t>(view.texture->image - data->images);
}

// A node's local transform as translation / rotation / scale. Matrices (allowed by glTF for nodes that aren't animated)
// are split up assuming no shear, which glTF forbids for node matrices anyway.
void readLocalTransform(const cgltf_node& node, HierarchyNodeData& out) {
    if (!node.has_matrix) {
        if (node.has_translation) out.translation = glm::make_vec3(node.translation);
        if (node.has_rotation) out.rotation = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
        if (node.has_scale) out.scale = glm::make_vec3(node.scale);
        return;
    }
    const glm::mat4 matrix = glm::make_mat4(node.matrix);
    out.translation = glm::vec3(matrix[3]);
    glm::mat3 axes(matrix);
    out.scale = glm::vec3(glm::length(axes[0]), glm::length(axes[1]), glm::length(axes[2]));
    if (glm::determinant(axes) < 0.0f) {
        out.scale.x = -out.scale.x; // a mirroring matrix: keep the rotation proper, put the flip in the scale
    }
    for (int axis = 0; axis < 3; ++axis) {
        axes[axis] /= out.scale[axis] != 0.0f ? out.scale[axis] : 1.0f;
    }
    out.rotation = glm::normalize(glm::quat_cast(axes));
}

// Joint indices and weights of one primitive into the model's skin stream. Weights are renormalized: exporters
// sometimes write sums slightly off 1, which would scale the skinned vertex.
void readSkinAttributes(const cgltf_primitive& primitive, uint32_t firstVertex, std::vector<SkinVertex>& skinVertices) {
    const cgltf_accessor* joints = cgltf_find_accessor(&primitive, cgltf_attribute_type_joints, 0);
    const cgltf_accessor* weights = cgltf_find_accessor(&primitive, cgltf_attribute_type_weights, 0);
    if (!joints || !weights) {
        return;
    }
    const std::vector<float> w4 = unpack(weights, 4);
    for (cgltf_size v = 0; v < joints->count; ++v) {
        SkinVertex& out = skinVertices[firstVertex + v];
        cgltf_uint indices[4] = {};
        cgltf_accessor_read_uint(joints, v, indices, 4);
        out.joints = glm::uvec4(indices[0], indices[1], indices[2], indices[3]);
        out.weights = glm::vec4(w4[v * 4], w4[v * 4 + 1], w4[v * 4 + 2], w4[v * 4 + 3]);
        const float sum = out.weights.x + out.weights.y + out.weights.z + out.weights.w;
        out.weights = sum > 0.0f ? out.weights / sum : glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    }
}

void readAnimations(const cgltf_data* data, const std::string& display, ModelData& model, std::vector<std::string>& warnings) {
    for (cgltf_size a = 0; a < data->animations_count; ++a) {
        const cgltf_animation& source = data->animations[a];
        AnimationData clip;
        clip.name = source.name ? source.name : "animation" + std::to_string(a);
        for (cgltf_size c = 0; c < source.channels_count; ++c) {
            const cgltf_animation_channel& channel = source.channels[c];
            if (!channel.target_node || !channel.sampler) {
                continue;
            }
            AnimationChannelData out;
            out.node = static_cast<uint32_t>(channel.target_node - data->nodes);
            size_t components = 3;
            switch (channel.target_path) {
            case cgltf_animation_path_type_translation: out.path = AnimationPath::Translation; break;
            case cgltf_animation_path_type_rotation: out.path = AnimationPath::Rotation; components = 4; break;
            case cgltf_animation_path_type_scale: out.path = AnimationPath::Scale; break;
            default:
                warnings.push_back(display + ": animation '" + clip.name + "' animates morph weights (not supported), channel skipped");
                continue;
            }
            switch (channel.sampler->interpolation) {
            case cgltf_interpolation_type_step: out.interpolation = AnimationInterpolation::Step; break;
            case cgltf_interpolation_type_cubic_spline: out.interpolation = AnimationInterpolation::CubicSpline; break;
            default: out.interpolation = AnimationInterpolation::Linear; break;
            }
            out.times = unpack(channel.sampler->input, 1);
            out.values = unpack(channel.sampler->output, components);
            if (!out.times.empty()) {
                clip.duration = std::max(clip.duration, out.times.back());
            }
            clip.channels.push_back(std::move(out));
        }
        model.animations.push_back(std::move(clip));
    }
}

} // namespace

GltfImporter::Result GltfImporter::load(const std::filesystem::path& path) {
    Result result;
    const auto start = std::chrono::steady_clock::now();
    const std::string pathUtf8 = core::Paths::toUtf8(path);
    const std::string display = core::Paths::display(path);

    cgltf_options options{};
    options.file.read = readFile;
    options.file.release = releaseFile;
    cgltf_data* data = nullptr;
    cgltf_result status = cgltf_parse_file(&options, pathUtf8.c_str(), &data);
    if (status == cgltf_result_success) {
        status = cgltf_load_buffers(&options, data, pathUtf8.c_str());
    }
    if (status != cgltf_result_success) {
        result.error = display + ": " + resultName(status) +
                       (status == cgltf_result_file_not_found ? " (missing asset? run.bat downloads assets listed in Content/AssetManifest.json)" : "");
        cgltf_free(data);
        return result;
    }

    ModelData& model = result.model;
    model.name = core::Paths::toUtf8(path.stem());
    const auto parsed = std::chrono::steady_clock::now();

    // Materials.
    for (cgltf_size i = 0; i < data->materials_count; ++i) {
        const cgltf_material& source = data->materials[i];
        MaterialData material;
        material.name = source.name ? source.name : "material" + std::to_string(i);
        if (source.has_pbr_metallic_roughness) {
            const cgltf_pbr_metallic_roughness& pbr = source.pbr_metallic_roughness;
            material.baseColorFactor = glm::make_vec4(pbr.base_color_factor);
            material.baseColorTexture = imageIndex(pbr.base_color_texture, data);
            material.metallicFactor = pbr.metallic_factor;
            material.roughnessFactor = pbr.roughness_factor;
            material.metallicRoughnessTexture = imageIndex(pbr.metallic_roughness_texture, data);
        }
        material.normalTexture = imageIndex(source.normal_texture, data);
        material.normalScale = source.normal_texture.scale;
        material.emissiveFactor = glm::make_vec3(source.emissive_factor);
        material.emissiveTexture = imageIndex(source.emissive_texture, data);
        if (source.has_emissive_strength) {
            material.emissiveStrength = source.emissive_strength.emissive_strength;
        }
        if (source.has_transmission) {
            material.transmission = source.transmission.transmission_factor;
        }
        material.alphaMode = source.alpha_mode == cgltf_alpha_mode_mask    ? AlphaMode::Mask
                             : source.alpha_mode == cgltf_alpha_mode_blend ? AlphaMode::Blend
                                                                          : AlphaMode::Opaque;
        material.alphaCutoff = source.alpha_cutoff;
        material.doubleSided = source.double_sided;
        model.materials.push_back(material);
    }
    const uint32_t defaultMaterial = static_cast<uint32_t>(model.materials.size());
    bool usedDefaultMaterial = false;

    // Images: color textures are sRGB, everything else linear. Decoded in parallel (Sponza has 69 of them).
    std::vector<ImageData::Format> formats(data->images_count, ImageData::Format::Rgba8);
    for (const MaterialData& material : model.materials) {
        if (material.baseColorTexture >= 0) formats[static_cast<size_t>(material.baseColorTexture)] = ImageData::Format::Rgba8Srgb;
        if (material.emissiveTexture >= 0) formats[static_cast<size_t>(material.emissiveTexture)] = ImageData::Format::Rgba8Srgb;
    }
    model.images.resize(data->images_count);
    std::vector<std::string> imageErrors(data->images_count);
    std::vector<size_t> order(data->images_count);
    std::iota(order.begin(), order.end(), size_t{0});
    const std::filesystem::path folder = path.parent_path();
    std::for_each(std::execution::par, order.begin(), order.end(), [&](size_t i) {
        const cgltf_image& image = data->images[i];
        std::string name = image.name ? image.name : (image.uri ? image.uri : "image" + std::to_string(i));
        if (image.buffer_view) {
            const uint8_t* bytes = cgltf_buffer_view_data(image.buffer_view);
            ImageLoader::loadFromMemory(bytes, image.buffer_view->size, formats[i], display + ":" + name, model.images[i], imageErrors[i]);
        } else if (image.uri && std::strncmp(image.uri, "data:", 5) != 0) {
            std::string uri = image.uri;
            uri.resize(cgltf_decode_uri(uri.data()));
            ImageLoader::loadFile(folder / core::Paths::fromUtf8(uri), formats[i], model.images[i], imageErrors[i]);
        } else {
            imageErrors[i] = display + ": image '" + name + "' uses an embedded data URI (not supported)";
        }
    });
    for (const std::string& error : imageErrors) {
        if (!error.empty()) {
            result.warnings.push_back(error); // a missing texture shouldn't stop the model from loading
        }
    }
    const auto decoded = std::chrono::steady_clock::now();

    // Meshes and primitives. The skin stream is filled alongside and dropped again if no primitive has joints.
    std::vector<SkinVertex> skinVertices;
    bool hasSkinAttributes = false;
    size_t triangles = 0;
    for (cgltf_size m = 0; m < data->meshes_count; ++m) {
        const cgltf_mesh& sourceMesh = data->meshes[m];
        MeshData mesh;
        mesh.name = sourceMesh.name ? sourceMesh.name : "mesh" + std::to_string(m);
        for (cgltf_size p = 0; p < sourceMesh.primitives_count; ++p) {
            const cgltf_primitive& primitive = sourceMesh.primitives[p];
            const cgltf_accessor* positions = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0);
            if (primitive.type != cgltf_primitive_type_triangles || !positions) {
                result.warnings.push_back(display + ": mesh '" + mesh.name + "' has a non-triangle primitive, skipped");
                continue;
            }
            const cgltf_accessor* normals = cgltf_find_accessor(&primitive, cgltf_attribute_type_normal, 0);
            const cgltf_accessor* tangents = cgltf_find_accessor(&primitive, cgltf_attribute_type_tangent, 0);
            const cgltf_accessor* uvs = cgltf_find_accessor(&primitive, cgltf_attribute_type_texcoord, 0);

            PrimitiveData out;
            out.firstVertex = static_cast<uint32_t>(model.vertices.size());
            out.vertexCount = static_cast<uint32_t>(positions->count);
            out.firstIndex = static_cast<uint32_t>(model.indices.size());
            model.vertices.resize(model.vertices.size() + positions->count);
            skinVertices.resize(model.vertices.size());
            if (cgltf_find_accessor(&primitive, cgltf_attribute_type_joints, 0)) {
                hasSkinAttributes = true;
                readSkinAttributes(primitive, out.firstVertex, skinVertices);
            }

            const std::vector<float> p3 = unpack(positions, 3);
            const std::vector<float> n3 = normals ? unpack(normals, 3) : std::vector<float>();
            const std::vector<float> t4 = tangents ? unpack(tangents, 4) : std::vector<float>();
            const std::vector<float> uv2 = uvs ? unpack(uvs, 2) : std::vector<float>();
            out.boundsMin = glm::vec3(std::numeric_limits<float>::max());
            out.boundsMax = glm::vec3(-std::numeric_limits<float>::max());
            for (cgltf_size v = 0; v < positions->count; ++v) {
                Vertex& vertex = model.vertices[out.firstVertex + v];
                vertex.position = glm::vec3(p3[v * 3], p3[v * 3 + 1], p3[v * 3 + 2]);
                if (normals) vertex.normal = glm::vec3(n3[v * 3], n3[v * 3 + 1], n3[v * 3 + 2]);
                if (tangents) vertex.tangent = glm::vec4(t4[v * 4], t4[v * 4 + 1], t4[v * 4 + 2], t4[v * 4 + 3]);
                if (uvs) vertex.uv = glm::vec2(uv2[v * 2], uv2[v * 2 + 1]);
                out.boundsMin = glm::min(out.boundsMin, vertex.position);
                out.boundsMax = glm::max(out.boundsMax, vertex.position);
            }

            if (primitive.indices) {
                for (cgltf_size i = 0; i < primitive.indices->count; ++i) {
                    model.indices.push_back(static_cast<uint32_t>(cgltf_accessor_read_index(primitive.indices, i)));
                }
            } else {
                for (uint32_t i = 0; i < out.vertexCount; ++i) {
                    model.indices.push_back(i);
                }
            }
            out.indexCount = static_cast<uint32_t>(model.indices.size()) - out.firstIndex;
            triangles += out.indexCount / 3;

            const uint32_t* indices = model.indices.data() + out.firstIndex;
            if (!normals) {
                computeNormals(model.vertices, out.firstVertex, out.vertexCount, indices, out.indexCount);
            }
            if (!tangents) {
                computeTangents(model.vertices, out.firstVertex, out.vertexCount, indices, out.indexCount);
            }

            if (primitive.material) {
                out.material = static_cast<uint32_t>(primitive.material - data->materials);
            } else {
                out.material = defaultMaterial;
                usedDefaultMaterial = true;
            }
            mesh.primitives.push_back(static_cast<uint32_t>(model.primitives.size()));
            model.primitives.push_back(out);
        }
        model.meshes.push_back(std::move(mesh));
    }
    if (usedDefaultMaterial) {
        MaterialData fallback;
        fallback.name = "default";
        fallback.metallicFactor = 0.0f;
        fallback.roughnessFactor = 0.8f;
        model.materials.push_back(fallback);
    }

    if (hasSkinAttributes) {
        model.skinVertices = std::move(skinVertices);
    }

    // The whole node tree (joints need their parents, including non-joint ancestors such as a Z-up correction).
    model.hierarchy.resize(data->nodes_count);
    for (cgltf_size n = 0; n < data->nodes_count; ++n) {
        const cgltf_node& node = data->nodes[n];
        HierarchyNodeData& out = model.hierarchy[n];
        out.name = node.name ? node.name : "node" + std::to_string(n);
        out.parent = node.parent ? static_cast<int32_t>(node.parent - data->nodes) : -1;
        readLocalTransform(node, out);
        float world[16];
        cgltf_node_transform_world(&node, world);
        out.restGlobal = glm::make_mat4(world);
    }

    // Skins.
    for (cgltf_size s = 0; s < data->skins_count; ++s) {
        const cgltf_skin& source = data->skins[s];
        SkinData skin;
        skin.name = source.name ? source.name : "skin" + std::to_string(s);
        for (cgltf_size j = 0; j < source.joints_count; ++j) {
            skin.joints.push_back(static_cast<uint32_t>(source.joints[j] - data->nodes));
        }
        // No inverse bind matrices in the file means identity (glTF spec).
        skin.inverseBindMatrices.assign(source.joints_count, glm::mat4(1.0f));
        if (source.inverse_bind_matrices) {
            const std::vector<float> m16 = unpack(source.inverse_bind_matrices, 16);
            for (cgltf_size j = 0; j < source.joints_count && (j + 1) * 16 <= m16.size(); ++j) {
                skin.inverseBindMatrices[j] = glm::make_mat4(m16.data() + j * 16);
            }
        }
        model.skins.push_back(std::move(skin));
    }

    readAnimations(data, display, model, result.warnings);

    // Nodes: every node that places a mesh, with its full model-space transform.
    for (cgltf_size n = 0; n < data->nodes_count; ++n) {
        const cgltf_node& node = data->nodes[n];
        if (!node.mesh) {
            continue;
        }
        NodeData out;
        out.name = node.name ? node.name : "node" + std::to_string(n);
        out.mesh = static_cast<uint32_t>(node.mesh - data->meshes);
        float matrix[16];
        cgltf_node_transform_world(&node, matrix);
        out.transform = glm::make_mat4(matrix);
        out.hierarchyNode = static_cast<int32_t>(n);
        out.skin = node.skin ? static_cast<int32_t>(node.skin - data->skins) : -1;
        model.nodes.push_back(out);
    }

    model.boundsMin = glm::vec3(std::numeric_limits<float>::max());
    model.boundsMax = glm::vec3(-std::numeric_limits<float>::max());
    for (const NodeData& node : model.nodes) {
        for (uint32_t primitiveIndex : model.meshes[node.mesh].primitives) {
            const PrimitiveData& primitive = model.primitives[primitiveIndex];
            for (int corner = 0; corner < 8; ++corner) {
                const glm::vec3 local((corner & 1) ? primitive.boundsMax.x : primitive.boundsMin.x,
                                      (corner & 2) ? primitive.boundsMax.y : primitive.boundsMin.y,
                                      (corner & 4) ? primitive.boundsMax.z : primitive.boundsMin.z);
                const glm::vec3 world = glm::vec3(node.transform * glm::vec4(local, 1.0f));
                model.boundsMin = glm::min(model.boundsMin, world);
                model.boundsMax = glm::max(model.boundsMax, world);
            }
        }
    }
    cgltf_free(data);

    const auto end = std::chrono::steady_clock::now();
    auto ms = [](auto from, auto to) { return std::chrono::duration<double, std::milli>(to - from).count(); };
    core::Log::info("Loaded {}: {} meshes, {} primitives, {} triangles, {} images, {} materials, {} skins, {} animations ({:.0f} ms: "
                    "parse {:.0f}, images {:.0f}, geometry {:.0f})",
                    display, model.meshes.size(), model.primitives.size(), triangles, model.images.size(), model.materials.size(),
                    model.skins.size(), model.animations.size(), ms(start, end), ms(start, parsed), ms(parsed, decoded), ms(decoded, end));
    result.ok = true;
    return result;
}

} // namespace ghost::assets
