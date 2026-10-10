// Owns one instance of every registered technique: turns their setup() declarations into render graph passes at the
// right stage, creates their pipelines, tracks enable/disable, and applies a scene's "techniques" settings.
#pragma once

#include "Graphics/TechniqueRuntime/Technique.h"
#include "Graphics/TechniqueRuntime/TechniqueInstance.h"

#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

namespace ghost::graphics::vulkan {
class Device;
class DeletionQueue;
} // namespace ghost::graphics::vulkan
namespace ghost::graphics::shader {
class PipelineLibrary;
}
namespace ghost::graphics::rendergraph {
class RenderGraph;
}
namespace ghost::graphics::scene {
class GpuScene;
}

namespace ghost::graphics::techniques {

class TechniqueBuilder;

class TechniqueManager {
public:
    TechniqueManager() = default;
    ~TechniqueManager();
    TechniqueManager(const TechniqueManager&) = delete;
    TechniqueManager& operator=(const TechniqueManager&) = delete;

    // Instantiates every technique in the registry (all disabled).
    void initialize(shader::PipelineLibrary& pipelines, const vulkan::Device& device, vulkan::DeletionQueue& deletionQueue);
    void shutdown();

    // Adds the passes of every enabled technique of `stage`, in registration order. Called while building the graph.
    void addPasses(rendergraph::RenderGraph& graph, const scene::GpuScene& scene, TechniqueStage stage);
    // True when a technique was enabled or disabled since the last call: the render graph must be rebuilt.
    bool enabledStateChanged();

    // A scene's "techniques" block: { "RayQueryNormals": { "enabled": true, "params": { "blend": 0.5 } } }.
    // Every technique first returns to its defaults (disabled), so a scene always looks the same. Unknown technique or
    // parameter names are logged as warnings with `source` (the scene file) in the message.
    void applySettings(const nlohmann::json& settings, const std::string& source);

    std::vector<Technique*> techniques() const;
    Technique* find(const std::string& id) const;

private:
    void createPipelines(TechniqueInstance& instance, const TechniqueBuilder& builder);
    void addTechniquePasses(rendergraph::RenderGraph& graph, const scene::GpuScene& scene, TechniqueInstance& instance);

    shader::PipelineLibrary* m_pipelines = nullptr;
    const vulkan::Device* m_device = nullptr;
    vulkan::DeletionQueue* m_deletionQueue = nullptr;
    std::vector<std::unique_ptr<TechniqueInstance>> m_instances;
};

} // namespace ghost::graphics::techniques
