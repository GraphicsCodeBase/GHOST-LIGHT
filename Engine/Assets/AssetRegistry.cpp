// AssetRegistry: dispatches to the glTF importer, procedural meshes or the HDR loader, and caches the results.
#include "Assets/AssetRegistry.h"

#include "Assets/GltfImporter.h"
#include "Assets/ImageLoader.h"
#include "Assets/ProceduralMeshes.h"
#include "Core/Paths.h"

namespace ghost::assets {

ModelHandle AssetRegistry::loadModel(const std::string& reference, std::string& error, std::vector<std::string>* warnings) {
    const auto found = m_modelLookup.find(reference);
    if (found != m_modelLookup.end()) {
        return found->second;
    }

    std::unique_ptr<ModelData> model;
    if (ProceduralMeshes::isProcedural(reference)) {
        std::optional<ModelData> generated = ProceduralMeshes::create(reference);
        if (!generated) {
            error = "unknown procedural model '" + reference + "' (available: procedural:CornellBox, procedural:Cube, procedural:Sphere, procedural:Plane)";
            return kInvalidModel;
        }
        model = std::make_unique<ModelData>(std::move(*generated));
    } else {
        GltfImporter::Result imported = GltfImporter::load(core::Paths::resolveContent(reference));
        if (warnings) {
            warnings->insert(warnings->end(), imported.warnings.begin(), imported.warnings.end());
        }
        if (!imported.ok) {
            error = imported.error;
            return kInvalidModel;
        }
        model = std::make_unique<ModelData>(std::move(imported.model));
        model->reference = reference;
    }

    m_models.push_back(std::move(model));
    const ModelHandle handle = static_cast<ModelHandle>(m_models.size() - 1);
    m_modelLookup.emplace(reference, handle);
    ++m_revision;
    return handle;
}

const ModelData* AssetRegistry::model(ModelHandle handle) const {
    return handle < m_models.size() ? m_models[handle].get() : nullptr;
}

ModelData* AssetRegistry::model(ModelHandle handle) {
    return handle < m_models.size() ? m_models[handle].get() : nullptr;
}

EnvironmentHandle AssetRegistry::loadEnvironment(const std::string& reference, std::string& error) {
    const auto found = m_environmentLookup.find(reference);
    if (found != m_environmentLookup.end()) {
        return found->second;
    }
    auto image = std::make_unique<ImageData>();
    if (!ImageLoader::loadHdrFile(core::Paths::resolveContent(reference), *image, error)) {
        return kInvalidEnvironment;
    }
    m_environments.push_back(std::move(image));
    const EnvironmentHandle handle = static_cast<EnvironmentHandle>(m_environments.size() - 1);
    m_environmentLookup.emplace(reference, handle);
    ++m_revision;
    return handle;
}

const ImageData* AssetRegistry::environment(EnvironmentHandle handle) const {
    return handle < m_environments.size() ? m_environments[handle].get() : nullptr;
}

void AssetRegistry::clear() {
    m_models.clear();
    m_modelLookup.clear();
    m_environments.clear();
    m_environmentLookup.clear();
    ++m_revision;
}

} // namespace ghost::assets
