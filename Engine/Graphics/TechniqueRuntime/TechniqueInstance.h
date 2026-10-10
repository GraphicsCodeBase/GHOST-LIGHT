// One live technique plus what the runtime keeps for it: its pipelines, change tracking and already-reported problems.
#pragma once

#include "Graphics/RayTracing/RayTracingPipeline.h"
#include "Graphics/ShaderCompiler/PipelineLibrary.h"
#include "Graphics/TechniqueRuntime/Technique.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace ghost::graphics::techniques {

struct TechniqueInstance {
    std::unique_ptr<Technique> technique;
    std::unordered_map<std::string, shader::PipelineHandle> computePipelines;
    std::unordered_map<std::string, std::unique_ptr<raytracing::RayTracingPipeline>> rayPipelines;
    bool wasEnabled = false;
    uint64_t lastSceneRevision = UINT64_MAX;
    uint64_t lastLightRevision = UINT64_MAX;
    bool sceneChanged = false;
    bool lightsChanged = false;
    std::unordered_set<std::string> reported; // each problem is logged once, not every frame
};

} // namespace ghost::graphics::techniques
