// Names one shader entry point: the Slang file, the function, and its pipeline stage.
#pragma once

#include <filesystem>
#include <string>

namespace ghost::graphics::shader {

enum class ShaderStage { Vertex, Fragment, Compute, RayGeneration, Miss, ClosestHit, AnyHit, Intersection };

// The function must carry a matching [shader("...")] attribute in the Slang source.
struct ShaderEntryPoint {
    std::filesystem::path file; // absolute
    std::string function;
    ShaderStage stage = ShaderStage::Compute;
};

} // namespace ghost::graphics::shader
