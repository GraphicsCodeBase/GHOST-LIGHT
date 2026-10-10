// What a technique sees in execute(): bindless indices and sizes of the textures it declared (by name), buffer
// addresses, the frame constants address for ShaderLibrary/Scene.slang, change flags, and dispatch/trace helpers.
#pragma once

#include "Graphics/RenderGraph/GraphTypes.h"
#include "Graphics/RenderGraph/PassContext.h"

#include <cstdint>
#include <string>
#include <unordered_map>

#include <volk.h>

namespace ghost::graphics::scene {
class GpuScene;
}
namespace ghost::graphics::shader {
class PipelineLibrary;
}

namespace ghost::graphics::techniques {

struct TechniqueInstance;

class TechniqueContext {
public:
    // Index of the pass being recorded, in the order setup() declared them.
    uint32_t pass() const { return m_passIndex; }

    uint32_t sampled(const std::string& texture) const;  // read(): index into gTextures[]
    uint32_t storage(const std::string& texture) const;  // write()/readWrite()/history(): index into gStorageImages[]
    uint32_t previous(const std::string& texture) const; // history(): last frame's copy, index into gTextures[]
    bool historyValid(const std::string& texture) const; // false right after the history texture was (re)created
    VkExtent2D extent(const std::string& texture) const;
    VkExtent2D renderSize() const;
    VkDeviceAddress buffer(const std::string& name) const;

    // FrameConstants* in shaders (import Scene;): camera, geometry, materials, lights, the TLAS index.
    VkDeviceAddress frameConstants() const;
    uint64_t frameIndex() const { return m_pass.frameIndex(); }
    // True on the first frame the technique runs and whenever the scene (or only its lights) changed since last frame.
    bool sceneChanged() const { return m_sceneChanged; }
    bool lightsChanged() const { return m_lightsChanged; }

    // The struct must match the shader's push constant struct exactly (scalar layout, at most 256 bytes).
    template <typename T>
    void pushConstants(const T& data) const {
        m_pass.pushConstants(data);
    }
    // One thread per pixel of `size` with the named compute pipeline (skipped while its shader doesn't compile).
    void dispatch(const std::string& pipeline, VkExtent2D size, uint32_t groupX = 8, uint32_t groupY = 8) const;
    // One ray generation invocation per pixel of `size` with the named ray tracing pipeline.
    void traceRays(const std::string& pipeline, VkExtent2D size) const;

    // Escape hatch for anything the helpers don't cover.
    VkCommandBuffer cmd() const { return m_pass.cmd(); }

private:
    friend class TechniqueManager;

    struct Resources {
        std::unordered_map<std::string, rendergraph::TextureHandle> sampled;
        std::unordered_map<std::string, rendergraph::TextureHandle> storage;
        std::unordered_map<std::string, rendergraph::TextureHandle> previous;
        std::unordered_map<std::string, rendergraph::BufferHandle> buffers;
    };

    TechniqueContext(rendergraph::PassContext& pass, TechniqueInstance& instance, shader::PipelineLibrary& pipelines, uint32_t passIndex,
                     rendergraph::PassKind kind, const Resources& resources, const scene::GpuScene& scene);
    const rendergraph::TextureHandle* find(const std::unordered_map<std::string, rendergraph::TextureHandle>& map, const std::string& name,
                                           const char* declaredWith) const;
    void reportOnce(const std::string& message) const;

    rendergraph::PassContext& m_pass;
    TechniqueInstance& m_instance;
    shader::PipelineLibrary& m_pipelines;
    uint32_t m_passIndex;
    rendergraph::PassKind m_kind;
    const Resources& m_resources;
    const scene::GpuScene& m_scene;
    bool m_sceneChanged;
    bool m_lightsChanged;
};

} // namespace ghost::graphics::techniques
