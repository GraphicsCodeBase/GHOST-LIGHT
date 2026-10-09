// Imports glTF 2.0 (.gltf + .bin + images, or .glb) into ModelData with cgltf; images are decoded in parallel.
#pragma once

#include "Assets/ModelData.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ghost::assets {

class GltfImporter {
public:
    struct Result {
        bool ok = false;
        ModelData model;
        std::string error;
        std::vector<std::string> warnings;
    };

    static Result load(const std::filesystem::path& path);
};

} // namespace ghost::assets
