// What a technique declares in setup(): its passes, the textures and buffers each pass uses (by name, shared with the
// engine's passes and other techniques), and its pipelines (shader paths relative to the technique's folder).
#pragma once

#include "Graphics/RenderGraph/GraphTypes.h"

#include <string>
#include <vector>

#include <volk.h>

namespace ghost::graphics::techniques {

class TechniqueBuilder {
public:
    using Scale = rendergraph::Scale;
    using Lifetime = rendergraph::Lifetime;

    // Compute passes run compute pipelines (ray queries included); ray tracing passes run ray tracing pipelines.
    enum class PassType { Compute, RayTracing };

    struct ShaderRef {
        std::string file;  // relative to the technique folder, e.g. "Shaders/Rtao.slang"
        std::string entry; // function with a matching [shader("...")] attribute
    };
    struct HitGroup {
        ShaderRef closestHit;
        ShaderRef anyHit; // leave file empty when not needed (alpha testing needs one)
    };
    struct RayPipelineDesc {
        ShaderRef rayGeneration;
        std::vector<ShaderRef> misses;
        std::vector<HitGroup> hitGroups;
    };

    // Starts the next pass. Single-pass techniques can skip it: their pass is named after the technique, and its type
    // is RayTracing if the technique declares a ray pipeline, Compute otherwise.
    TechniqueBuilder& pass(const std::string& name, PassType type = PassType::Compute);

    // Textures of the current pass.
    TechniqueBuilder& read(const std::string& texture);                                          // sampled: ctx.sampled()
    TechniqueBuilder& write(const std::string& texture);                                         // existing, storage: ctx.storage()
    TechniqueBuilder& write(const std::string& texture, VkFormat format, Scale scale = Scale::Full); // created by this pass
    TechniqueBuilder& readWrite(const std::string& texture);                                     // storage, keeps contents
    // Two copies swapped every frame: write this frame's (ctx.storage), read last frame's (ctx.previous).
    TechniqueBuilder& history(const std::string& texture, VkFormat format, Scale scale = Scale::Full);
    // A GPU buffer reached by address in shaders (ctx.buffer()), e.g. reservoirs or hash grids.
    TechniqueBuilder& buffer(const std::string& name, VkDeviceSize bytes, Lifetime lifetime = Lifetime::Persistent);

    // Pipelines, created once and hot-reloaded like every engine shader.
    void computePipeline(const std::string& name, const std::string& file, const std::string& entry = "main");
    void rayPipeline(const std::string& name, const RayPipelineDesc& desc);

private:
    friend class TechniqueManager;

    enum class AccessKind { Read, Write, Create, ReadWrite, History, Buffer };
    struct Access {
        AccessKind kind;
        std::string name;
        rendergraph::TextureDesc desc;
        VkDeviceSize bytes = 0;
        Lifetime lifetime = Lifetime::Persistent;
    };
    struct Pass {
        std::string name;
        PassType type = PassType::Compute;
        bool implicit = false;
        std::vector<Access> accesses;
    };
    struct ComputeDecl {
        std::string name;
        ShaderRef shader;
    };
    struct RayDecl {
        std::string name;
        RayPipelineDesc desc;
    };

    explicit TechniqueBuilder(std::string defaultPassName) : m_defaultPassName(std::move(defaultPassName)) {}
    Pass& current();
    void finish(); // resolves the implicit pass type

    std::string m_defaultPassName;
    std::vector<Pass> m_passes;
    std::vector<ComputeDecl> m_compute;
    std::vector<RayDecl> m_ray;
};

} // namespace ghost::graphics::techniques
