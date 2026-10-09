// Loads and caches models and environment maps by the name scenes use for them; hands out stable handles.
#pragma once

#include "Assets/ImageData.h"
#include "Assets/ModelData.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ghost::assets {

using ModelHandle = uint32_t;
using EnvironmentHandle = uint32_t;
inline constexpr ModelHandle kInvalidModel = UINT32_MAX;
inline constexpr EnvironmentHandle kInvalidEnvironment = UINT32_MAX;

class AssetRegistry {
public:
    // reference: a path relative to Content/ ("Assets/Models/Sponza/Sponza.gltf") or "procedural:<Name>".
    // Loaded once; later calls return the same handle. On failure returns kInvalidModel and fills error
    // (failures are not cached, so fixing the file and reloading the scene works).
    ModelHandle loadModel(const std::string& reference, std::string& error, std::vector<std::string>* warnings = nullptr);
    const ModelData* model(ModelHandle handle) const;
    ModelData* model(ModelHandle handle);
    size_t modelCount() const { return m_models.size(); }

    // Equirectangular .hdr relative to Content/.
    EnvironmentHandle loadEnvironment(const std::string& reference, std::string& error);
    const ImageData* environment(EnvironmentHandle handle) const;

    // Forgets everything (handles become invalid). Bumps revision().
    void clear();
    // Changes whenever models or environments are added or cleared (the GPU scene re-uploads on change).
    uint64_t revision() const { return m_revision; }

private:
    std::vector<std::unique_ptr<ModelData>> m_models;
    std::unordered_map<std::string, ModelHandle> m_modelLookup;
    std::vector<std::unique_ptr<ImageData>> m_environments;
    std::unordered_map<std::string, EnvironmentHandle> m_environmentLookup;
    uint64_t m_revision = 1;
};

} // namespace ghost::assets
