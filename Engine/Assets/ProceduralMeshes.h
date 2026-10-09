// Built-in models generated in code (no download): the Cornell box for path tracer validation and basic shapes.
// Scenes refer to them as "procedural:CornellBox", "procedural:Cube", "procedural:Sphere", "procedural:Plane".
#pragma once

#include "Assets/ModelData.h"

#include <optional>
#include <string_view>
#include <vector>

namespace ghost::assets {

class ProceduralMeshes {
public:
    static constexpr std::string_view kPrefix = "procedural:";

    static bool isProcedural(std::string_view reference) { return reference.starts_with(kPrefix); }
    // Returns nothing for an unknown name.
    static std::optional<ModelData> create(std::string_view reference);
    static std::vector<std::string_view> names();
};

} // namespace ghost::assets
