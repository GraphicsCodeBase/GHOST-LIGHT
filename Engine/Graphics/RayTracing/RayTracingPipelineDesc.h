// Description of a ray tracing pipeline: one ray generation shader, miss shaders and triangle hit groups.
#pragma once

#include "Graphics/ShaderCompiler/ShaderEntryPoint.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ghost::graphics::raytracing {

struct RayTracingHitGroup {
    std::optional<shader::ShaderEntryPoint> closestHit;
    std::optional<shader::ShaderEntryPoint> anyHit; // alpha testing; leave empty for opaque-only hit groups
};

// Shader binding table order follows this description: miss index i = misses[i], hit group i = hitGroups[i]
// (TraceRay's missIndex and sbtRecordOffset select them).
struct RayTracingPipelineDesc {
    std::string name;
    shader::ShaderEntryPoint rayGeneration;
    std::vector<shader::ShaderEntryPoint> misses;
    std::vector<RayTracingHitGroup> hitGroups;
    uint32_t maxRecursionDepth = 1; // 1 = TraceRay only from ray generation (loops instead of recursion)
};

} // namespace ghost::graphics::raytracing
